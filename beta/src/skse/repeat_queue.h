#pragma once
#include <cmath>
#include <unordered_set>
#include <vector>

// Shared by the game observer and the executable regression test.
template<class ID, class Job>
class RepeatQueue
{
public:
    bool MarkSeen(ID id) { return _seen.insert(id).second; }
    void Forget(ID id) { _seen.erase(id); }
    void Add(const Job& job) { _pending.push_back(job); }
    void Reset() { _seen.clear(); _pending.clear(); }
    std::vector<Job> Advance(float delta, bool paused)
    {
        std::vector<Job> ready;
        if (paused || !std::isfinite(delta) || delta <= 0) {
            return ready;
        }
        for (auto it = _pending.begin(); it != _pending.end();) {
            it->remaining -= delta;
            if (it->remaining <= 0) {
                ready.push_back(*it);
                it = _pending.erase(it);
            } else {
                ++it;
            }
        }
        return ready;
    }
private:
    std::unordered_set<ID> _seen;
    std::vector<Job> _pending;
};
