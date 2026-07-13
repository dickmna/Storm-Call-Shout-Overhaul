#include <RE/Skyrim.h>
#include <RE/B/BSVisit.h>
#include <SKSE/SKSE.h>

#include <Windows.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <string>

namespace
{
	constexpr auto PLUGIN_NAME = "SCSOProjectileBounds";
	constexpr auto INI_PATH = ".\\Data\\SKSE\\Plugins\\SCSOProjectileBounds.ini";
	constexpr auto SKYRIM_ESM = "Skyrim.esm";
	constexpr RE::FormID STORM_PROJECTILE = 0x000E4CB5;

	struct Settings
	{
		bool enable{ true };
		bool patchProjectileBounds{ true };
		bool forceProjectileAlwaysDraw{ true };
		float projectileBoundRadius{ 18000.0F };
	};

	Settings g_settings;
	RE::BGSProjectile* g_stormProjectile{ nullptr };
	bool g_projectileHooksInstalled{ false };

	std::wstring GetPluginDirectory()
	{
		HMODULE module{};
		const auto flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
		if (!GetModuleHandleExW(flags, reinterpret_cast<LPCWSTR>(&GetPluginDirectory), std::addressof(module))) {
			return {};
		}

		wchar_t path[MAX_PATH]{};
		if (GetModuleFileNameW(module, path, static_cast<DWORD>(std::size(path))) == 0) {
			return {};
		}

		std::wstring directory{ path };
		const auto slash = directory.find_last_of(L"\\/");
		if (slash == std::wstring::npos) {
			return {};
		}

		directory.resize(slash);
		return directory;
	}

	void PreloadLocalDependency(const wchar_t* a_name)
	{
		const auto directory = GetPluginDirectory();
		if (directory.empty()) {
			return;
		}

		const auto fullPath = directory + L"\\" + a_name;
		if (!LoadLibraryW(fullPath.c_str())) {
			OutputDebugStringW((L"SCSOProjectileBounds failed to preload dependency: " + fullPath + L"\n").c_str());
		}
	}

	void PreloadLocalDependencies()
	{
		PreloadLocalDependency(L"fmt.dll");
		PreloadLocalDependency(L"spdlog.dll");
	}

	bool ReadBool(const char* a_section, const char* a_key, bool a_default)
	{
		return GetPrivateProfileIntA(a_section, a_key, a_default ? 1 : 0, INI_PATH) != 0;
	}

	float ReadFloat(const char* a_section, const char* a_key, float a_default)
	{
		char buffer[64]{};
		const auto defaultValue = std::to_string(a_default);
		GetPrivateProfileStringA(
			a_section,
			a_key,
			defaultValue.c_str(),
			buffer,
			static_cast<DWORD>(std::size(buffer)),
			INI_PATH);
		return std::strtof(buffer, nullptr);
	}

	void LoadSettings()
	{
		g_settings.enable = ReadBool("General", "bEnable", g_settings.enable);
		g_settings.patchProjectileBounds = ReadBool(
			"General", "bPatchProjectileBounds", g_settings.patchProjectileBounds);
		g_settings.forceProjectileAlwaysDraw = ReadBool(
			"General", "bForceProjectileAlwaysDraw", g_settings.forceProjectileAlwaysDraw);
		g_settings.projectileBoundRadius = std::max(
			512.0F,
			ReadFloat("Visual", "fProjectileBoundRadius", g_settings.projectileBoundRadius));
	}

	template <class T>
	T* LookupSkyrimForm(RE::FormID a_formID)
	{
		const auto dataHandler = RE::TESDataHandler::GetSingleton();
		return dataHandler ? dataHandler->LookupForm<T>(a_formID, SKYRIM_ESM) : nullptr;
	}

	bool ResolveForms()
	{
		g_stormProjectile = LookupSkyrimForm<RE::BGSProjectile>(STORM_PROJECTILE);
		if (!g_stormProjectile) {
			SKSE::log::error("Failed to resolve ShockBoltAimStorm projectile {:08X}", STORM_PROJECTILE);
			return false;
		}

		SKSE::log::info("Resolved ShockBoltAimStorm projectile {:08X}", g_stormProjectile->GetFormID());
		return true;
	}

	bool IsStormProjectile(RE::Projectile* a_projectile)
	{
		return a_projectile && g_stormProjectile && a_projectile->GetProjectileBase() == g_stormProjectile;
	}

	void ExpandStormProjectileBounds(RE::Projectile* a_projectile, RE::NiAVObject* a_root)
	{
		if (!g_settings.patchProjectileBounds || !IsStormProjectile(a_projectile)) {
			return;
		}

		auto* root = a_root ? a_root : a_projectile->Get3D();
		if (!root) {
			return;
		}

		const auto radius = g_settings.projectileBoundRadius;
		RE::BSVisit::TraverseScenegraphObjects(
			root,
			[&](RE::NiAVObject* a_object) -> RE::BSVisit::BSVisitControl {
				if (!a_object) {
					return RE::BSVisit::BSVisitControl::kContinue;
				}

				auto& flags = a_object->GetFlags();
				flags.set(RE::NiAVObject::Flag::kFixedBound, RE::NiAVObject::Flag::kForceUpdate);
				if (g_settings.forceProjectileAlwaysDraw) {
					flags.set(RE::NiAVObject::Flag::kAlwaysDraw);
				}

				a_object->SetAppCulled(false);
				a_object->worldBound.center = a_object->world.translate;
				a_object->worldBound.radius = std::max(a_object->worldBound.radius, radius);
				return RE::BSVisit::BSVisitControl::kContinue;
			});

		root->SetAppCulled(false);
		root->worldBound.center = root->world.translate;
		root->worldBound.radius = std::max(root->worldBound.radius, radius);
	}

	struct ProjectileBoundsHooks
	{
		using PostLoad3D_t = void (*)(RE::Projectile*, RE::NiAVObject*);
		using Update3D_t = void (*)(RE::Projectile*);

		static inline PostLoad3D_t projectilePostLoad3D{ nullptr };
		static inline Update3D_t projectileUpdate3D{ nullptr };
		static inline PostLoad3D_t beamPostLoad3D{ nullptr };
		static inline Update3D_t beamUpdate3D{ nullptr };
		static inline PostLoad3D_t missilePostLoad3D{ nullptr };
		static inline Update3D_t missileUpdate3D{ nullptr };

		static void HookedProjectilePostLoad3D(RE::Projectile* a_projectile, RE::NiAVObject* a_root)
		{
			projectilePostLoad3D(a_projectile, a_root);
			ExpandStormProjectileBounds(a_projectile, a_root);
		}

		static void HookedProjectileUpdate3D(RE::Projectile* a_projectile)
		{
			projectileUpdate3D(a_projectile);
			ExpandStormProjectileBounds(a_projectile, nullptr);
		}

		static void HookedBeamPostLoad3D(RE::Projectile* a_projectile, RE::NiAVObject* a_root)
		{
			beamPostLoad3D(a_projectile, a_root);
			ExpandStormProjectileBounds(a_projectile, a_root);
		}

		static void HookedBeamUpdate3D(RE::Projectile* a_projectile)
		{
			beamUpdate3D(a_projectile);
			ExpandStormProjectileBounds(a_projectile, nullptr);
		}

		static void HookedMissilePostLoad3D(RE::Projectile* a_projectile, RE::NiAVObject* a_root)
		{
			missilePostLoad3D(a_projectile, a_root);
			ExpandStormProjectileBounds(a_projectile, a_root);
		}

		static void HookedMissileUpdate3D(RE::Projectile* a_projectile)
		{
			missileUpdate3D(a_projectile);
			ExpandStormProjectileBounds(a_projectile, nullptr);
		}
	};

	void InstallProjectileBoundsHooks()
	{
		if (g_projectileHooksInstalled || !g_settings.patchProjectileBounds) {
			return;
		}

		REL::Relocation<std::uintptr_t> projectileVtbl{ RE::VTABLE_Projectile[0] };
		ProjectileBoundsHooks::projectilePostLoad3D = reinterpret_cast<ProjectileBoundsHooks::PostLoad3D_t>(
			projectileVtbl.write_vfunc(0xAA, ProjectileBoundsHooks::HookedProjectilePostLoad3D));
		ProjectileBoundsHooks::projectileUpdate3D = reinterpret_cast<ProjectileBoundsHooks::Update3D_t>(
			projectileVtbl.write_vfunc(0xAD, ProjectileBoundsHooks::HookedProjectileUpdate3D));

		REL::Relocation<std::uintptr_t> beamVtbl{ RE::VTABLE_BeamProjectile[0] };
		ProjectileBoundsHooks::beamPostLoad3D = reinterpret_cast<ProjectileBoundsHooks::PostLoad3D_t>(
			beamVtbl.write_vfunc(0xAA, ProjectileBoundsHooks::HookedBeamPostLoad3D));
		ProjectileBoundsHooks::beamUpdate3D = reinterpret_cast<ProjectileBoundsHooks::Update3D_t>(
			beamVtbl.write_vfunc(0xAD, ProjectileBoundsHooks::HookedBeamUpdate3D));

		REL::Relocation<std::uintptr_t> missileVtbl{ RE::VTABLE_MissileProjectile[0] };
		ProjectileBoundsHooks::missilePostLoad3D = reinterpret_cast<ProjectileBoundsHooks::PostLoad3D_t>(
			missileVtbl.write_vfunc(0xAA, ProjectileBoundsHooks::HookedMissilePostLoad3D));
		ProjectileBoundsHooks::missileUpdate3D = reinterpret_cast<ProjectileBoundsHooks::Update3D_t>(
			missileVtbl.write_vfunc(0xAD, ProjectileBoundsHooks::HookedMissileUpdate3D));

		g_projectileHooksInstalled = true;
		SKSE::log::info("Installed projectile bounds hook for ShockBoltAimStorm only");
	}

	void OnDataLoaded()
	{
		LoadSettings();
		if (!g_settings.enable) {
			SKSE::log::info("Plugin disabled by INI");
			return;
		}

		if (ResolveForms()) {
			InstallProjectileBoundsHooks();
		}
	}

	void MessageHandler(SKSE::MessagingInterface::Message* a_message)
	{
		if (a_message && a_message->type == SKSE::MessagingInterface::kDataLoaded) {
			OnDataLoaded();
		}
	}

	void SetupLog()
	{
		auto path = SKSE::log::log_directory();
		if (!path) {
			return;
		}

		*path /= "SCSOProjectileBounds.log";
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
		auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));
		log->set_level(spdlog::level::debug);
		log->flush_on(spdlog::level::info);
		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
	}
}

SKSEPluginInfo(
	.Version = { 0, 1, 0, 0 },
	.Name = PLUGIN_NAME,
	.Author = "dickmna",
	.StructCompatibility = SKSE::StructCompatibility::Independent,
	.RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary,
	.MinimumSKSEVersion = { 2, 2, 0, 0 }
)

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	PreloadLocalDependencies();
	SetupLog();
	SKSE::log::info("{} loaded", PLUGIN_NAME);

	const auto messaging = SKSE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(MessageHandler)) {
		SKSE::log::error("Failed to register SKSE messaging listener");
		return false;
	}

	return true;
}
