#include <RE/Skyrim.h>
#include <RE/B/BSVisit.h>
#include <SKSE/SKSE.h>
#include <Windows.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>
#include "repeat_queue.h"

namespace
{
    constexpr auto PLUGIN_NAME = "SCSOStormCall";
    constexpr auto INI_PATH = ".\\Data\\SKSE\\Plugins\\SCSOStormCall.ini";
    RE::TESShout* g_stormCall{};
    RE::BGSProjectile* g_stormProjectile{};
    bool g_installed{};
    thread_local bool g_launchingRepeat{};

    struct Settings
    {
        bool enable{ true };
        bool bounds{ true };
        bool alwaysDraw{ true };
        float radius{ 18000.0F };
        float staggerMin{ 0.12F };
        float staggerMax{ 0.35F };
    } g_settings;

    float ReadFloat(const char* section, const char* key, float fallback)
    {
        char value[64]{};
        const auto defaultValue = std::to_string(fallback);
        GetPrivateProfileStringA(section, key, defaultValue.c_str(), value, sizeof(value), INI_PATH);
        const auto parsed = std::strtof(value, nullptr);
        return std::isfinite(parsed) ? parsed : fallback;
    }

    void LoadSettings()
    {
        g_settings.enable = GetPrivateProfileIntA("General", "bEnable", 1, INI_PATH) != 0;
        g_settings.bounds = GetPrivateProfileIntA("General", "bPatchProjectileBounds", 1, INI_PATH) != 0;
        g_settings.alwaysDraw = GetPrivateProfileIntA("General", "bForceProjectileAlwaysDraw", 1, INI_PATH) != 0;
        g_settings.radius = std::clamp(ReadFloat("Visual", "fProjectileBoundRadius", 18000.0F), 512.0F, 50000.0F);
        g_settings.staggerMin = std::clamp(ReadFloat("Timing", "fPassStaggerMin", 0.12F), 0.01F, 2.0F);
        g_settings.staggerMax = std::clamp(ReadFloat("Timing", "fPassStaggerMax", 0.35F), g_settings.staggerMin, 2.0F);
    }

    bool IsStormProjectile(RE::Projectile* projectile)
    {
        return projectile && g_stormProjectile && projectile->GetProjectileBase() == g_stormProjectile;
    }

    void ExpandBounds(RE::Projectile* projectile, RE::NiAVObject* root)
    {
        if (!g_settings.bounds || !IsStormProjectile(projectile)) {
            return;
        }
        root = root ? root : projectile->Get3D();
        if (!root) {
            return;
        }
        RE::BSVisit::TraverseScenegraphObjects(root, [](RE::NiAVObject* object) {
            if (object) {
                auto& flags = object->GetFlags();
                flags.set(RE::NiAVObject::Flag::kFixedBound, RE::NiAVObject::Flag::kForceUpdate);
                if (g_settings.alwaysDraw) {
                    flags.set(RE::NiAVObject::Flag::kAlwaysDraw);
                }
                object->SetAppCulled(false);
                object->worldBound.center = object->world.translate;
                object->worldBound.radius = std::max(object->worldBound.radius, g_settings.radius);
            }
            return RE::BSVisit::BSVisitControl::kContinue;
        });
    }

    bool IsOutdoors(RE::TESObjectREFR* reference)
    {
        const auto cell = reference ? reference->GetParentCell() : nullptr;
        return cell && !cell->IsInteriorCell();
    }

    unsigned ActiveStormWord(RE::Actor* caster)
    {
        if (!caster || !g_stormCall) {
            return 0;
        }
        const auto effects = caster->GetActiveEffectList();
        if (!effects) {
            return 0;
        }
        unsigned word = 0;
        for (const auto active : *effects) {
            if (!active || active->flags.any(RE::ActiveEffect::Flag::kInactive, RE::ActiveEffect::Flag::kDispelled)) {
                continue;
            }
            for (unsigned index = 0; index < 3; ++index) {
                // Compare the final winning spell, rather than fixed vanilla spell IDs.
                if (active->spell == g_stormCall->variations[index].spell) {
                    word = std::max(word, index + 1);
                }
            }
        }
        return word;
    }

    struct Repeat
    {
        RE::ObjectRefHandle shooter;
        RE::ObjectRefHandle target;
        RE::ActorHandle caster;
        RE::NiPointer<RE::ActorCause> cause;
        RE::MagicItem* spell{};
        RE::BGSProjectile* base{};
        RE::NiPoint3 origin;
        RE::Projectile::ProjectileRot angles;
        RE::MagicSystem::CastingSource source{};
        float power{};
        float scale{};
        bool alwaysHit{};
        bool autoAim{};
        bool noDamageOutsideCombat{};
        float remaining{};
    };

    class Repeats
    {
    public:
        static Repeats& Get()
        {
            static Repeats instance;
            return instance;
        }

        void Reset()
        {
            std::scoped_lock lock(_mutex);
            _handles.clear();
            _queue.Reset();
        }

        void Observe(RE::Projectile* projectile)
        {
            if (g_launchingRepeat || !IsStormProjectile(projectile)) {
                return;
            }
            std::scoped_lock lock(_mutex);
            const auto id = projectile->GetFormID();
            if (!_queue.MarkSeen(id)) {
                return;
            }
            _handles.emplace(id, RE::ProjectileHandle{ projectile });

            const auto& data = projectile->GetProjectileRuntimeData();
            const auto shooter = data.shooter.get();
            const auto target = data.desiredTarget.get();
            const auto cause = data.actorCause;
            const auto caster = cause ? cause->actor.get() : RE::NiPointer<RE::Actor>{};
            auto* casterActor = caster ? caster.get() : (shooter ? shooter->As<RE::Actor>() : nullptr);
            const auto word = ActiveStormWord(casterActor);
            if (word < 2 || !data.spell || !shooter || !target ||
                !IsOutdoors(casterActor) || !IsOutdoors(target.get())) {
                return;
            }
            Repeat repeat;
            repeat.shooter = data.shooter;
            repeat.target = data.desiredTarget;
            repeat.caster = RE::ActorHandle{ casterActor };
            repeat.cause = cause;
            repeat.spell = data.spell;
            repeat.base = projectile->GetProjectileBase();
            repeat.origin = projectile->GetPosition();
            const auto angle = projectile->GetAngle();
            repeat.angles = { angle.x, angle.z };
            repeat.source = data.castingSource;
            repeat.power = data.power;
            repeat.scale = data.scale;
            repeat.alwaysHit = data.flags.any(RE::Projectile::Flags::kAlwaysHit);
            repeat.autoAim = data.flags.any(RE::Projectile::Flags::kAutoAim);
            repeat.noDamageOutsideCombat = data.flags.any(RE::Projectile::Flags::kNoDamageOutsideCombat);
            std::uniform_real_distribution<float> stagger(g_settings.staggerMin, g_settings.staggerMax);
            for (unsigned index = 1; index < word; ++index) {
                repeat.remaining += stagger(_random);
                _queue.Add(repeat);
            }
        }

        void Tick(float delta)
        {
            const auto ui = RE::UI::GetSingleton();
            if (!std::isfinite(delta) || delta <= 0 || (ui && ui->GameIsPaused())) {
                return;
            }
            std::vector<Repeat> ready;
            {
                std::scoped_lock lock(_mutex);
                std::erase_if(_handles, [&](const auto& entry) {
                    if (!entry.second.get()) {
                        _queue.Forget(entry.first);
                        return true;
                    }
                    return false;
                });
                ready = _queue.Advance(delta, false);
            }
            for (const auto& repeat : ready) {
                Launch(repeat);
            }
        }

    private:
        void Launch(const Repeat& repeat)
        {
            const auto caster = repeat.caster.get();
            const auto shooter = repeat.shooter.get();
            const auto target = repeat.target.get();
            if (!caster || !shooter || !target || !repeat.spell || !repeat.base ||
                !IsOutdoors(caster.get()) || !IsOutdoors(target.get()) ||
                !target->Is3DLoaded() || ActiveStormWord(caster.get()) == 0) {
                return;
            }
            if (const auto actor = target->As<RE::Actor>(); actor && actor->IsDead()) {
                return;
            }
            // Engine spell damage, perks, resistances and attribution stay in the engine.
            RE::Projectile::LaunchData data(repeat.base, caster.get(), repeat.origin, repeat.angles);
            data.shooter = shooter.get();
            data.desiredTarget = target.get();
            data.parentCell = target->GetParentCell();
            data.spell = repeat.spell;
            data.castingSource = repeat.source;
            if (const auto effect = repeat.spell->GetCostliestEffectItem()) {
                data.area = effect->GetArea();
            }
            data.power = repeat.power;
            data.scale = repeat.scale;
            data.alwaysHit = repeat.alwaysHit;
            data.autoAim = repeat.autoAim;
            data.noDamageOutsideCombat = repeat.noDamageOutsideCombat;
            data.useOrigin = true;
            RE::ProjectileHandle handle;
            struct Guard
            {
                Guard() { g_launchingRepeat = true; }
                ~Guard() { g_launchingRepeat = false; }
            } guard;
            RE::Projectile::Launch(std::addressof(handle), data);
            if (const auto projectile = handle.get()) {
                if (repeat.cause) {
                    projectile->SetActorCause(repeat.cause.get());
                }
                std::scoped_lock lock(_mutex);
                _queue.MarkSeen(projectile->GetFormID());
                _handles.emplace(projectile->GetFormID(), handle);
            }
        }

        std::mutex _mutex;
        std::unordered_map<RE::FormID, RE::ProjectileHandle> _handles;
        RepeatQueue<RE::FormID, Repeat> _queue;
        std::mt19937 _random{ std::random_device{}() };
    };

    struct Hooks
    {
        static inline void (*beamUpdate)(RE::Projectile*, float){};
        static inline void (*beamPostLoad)(RE::Projectile*, RE::NiAVObject*){};
        static inline void (*beamUpdate3D)(RE::Projectile*){};
        static inline void (*playerUpdate)(RE::Actor*, float){};

        static void BeamUpdate(RE::Projectile* projectile, float delta)
        {
            Repeats::Get().Observe(projectile);
            beamUpdate(projectile, delta);
        }
        static void BeamPostLoad(RE::Projectile* projectile, RE::NiAVObject* root)
        {
            beamPostLoad(projectile, root);
            ExpandBounds(projectile, root);
        }
        static void BeamUpdate3D(RE::Projectile* projectile)
        {
            beamUpdate3D(projectile);
            ExpandBounds(projectile, nullptr);
        }
        static void PlayerUpdate(RE::Actor* player, float delta)
        {
            playerUpdate(player, delta);
            Repeats::Get().Tick(delta);
        }
    };

    void MessageHandler(SKSE::MessagingInterface::Message* message)
    {
        if (!message) {
            return;
        }
        if (message->type == SKSE::MessagingInterface::kPreLoadGame ||
            message->type == SKSE::MessagingInterface::kNewGame) {
            Repeats::Get().Reset();
        }
        if (message->type != SKSE::MessagingInterface::kDataLoaded || g_installed) {
            return;
        }
        LoadSettings();
        const auto handler = RE::TESDataHandler::GetSingleton();
        if (!g_settings.enable || !handler) {
            return;
        }
        g_stormCall = handler->LookupForm<RE::TESShout>(0x7097D, "Skyrim.esm");
        g_stormProjectile = handler->LookupForm<RE::BGSProjectile>(0xE4CB5, "Skyrim.esm");
        if (!g_stormCall || !g_stormProjectile) {
            SKSE::log::error("Required Storm Call forms unavailable; hooks were not installed");
            return;
        }
        REL::Relocation<std::uintptr_t> beam{ RE::VTABLE_BeamProjectile[0] };
        REL::Relocation<std::uintptr_t> player{ RE::VTABLE_PlayerCharacter[0] };
        Hooks::beamUpdate = reinterpret_cast<decltype(Hooks::beamUpdate)>(beam.write_vfunc(0xAB, Hooks::BeamUpdate));
        Hooks::beamPostLoad = reinterpret_cast<decltype(Hooks::beamPostLoad)>(beam.write_vfunc(0xAA, Hooks::BeamPostLoad));
        Hooks::beamUpdate3D = reinterpret_cast<decltype(Hooks::beamUpdate3D)>(beam.write_vfunc(0xAD, Hooks::BeamUpdate3D));
        Hooks::playerUpdate = reinterpret_cast<decltype(Hooks::playerUpdate)>(player.write_vfunc(0xAD, Hooks::PlayerUpdate));
        g_installed = true;
        SKSE::log::info("Chained Storm Call beam observers; winning spells and primary cadence remain active");
    }
}

SKSEPluginVersion = []() constexpr {
    SKSE::PluginVersionData data;
    data.PluginVersion({ 3, 0, 1, 0 });
    data.PluginName(PLUGIN_NAME);
    data.AuthorName("dickmna");
    data.UsesAddressLibrary();
    data.UsesUpdatedStructs();
    data.CompatibleVersions({ SKSE::RUNTIME_SSE_1_7_104 });
    data.MinimumRequiredXSEVersion({ 2, 3, 1, 0 });
    return data;
}();

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    if (skse->RuntimeVersion() != SKSE::RUNTIME_SSE_1_7_104) {
        return false;
    }
    SKSE::Init(skse);
    if (auto path = SKSE::log::log_directory()) {
        *path /= "SCSOStormCall.log";
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
        auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));
        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);
        spdlog::set_default_logger(std::move(log));
    }
    const auto messaging = SKSE::GetMessagingInterface();
    return messaging && messaging->RegisterListener(MessageHandler);
}
