#include "../src/skse/repeat_queue.h"
#include <cassert>
#include <limits>
#include <iostream>
#ifdef NDEBUG
#error Regression assertions must stay enabled.
#endif

struct Job { int ordinal; float remaining; };
int main()
{
    RepeatQueue<unsigned, Job> queue;
    assert(queue.MarkSeen(100));
    assert(!queue.MarkSeen(100)); // The same original beam cannot queue twice.
    queue.Add({ 1, 0.12F });
    queue.Add({ 2, 0.47F });
    assert(queue.Advance(1.0F, true).empty()); // Menu pause preserves both delays.
    assert(queue.Advance(-1.0F, false).empty());
    assert(queue.Advance(std::numeric_limits<float>::quiet_NaN(), false).empty());
    assert(queue.Advance(0.10F, false).empty());
    auto first = queue.Advance(0.03F, false);
    assert(first.size() == 1 && first[0].ordinal == 1);
    assert(queue.MarkSeen(101)); // A launched replica is marked before its update.
    assert(!queue.MarkSeen(101)); // Its observer cannot cause recursive growth.
    assert(queue.Advance(0.20F, false).empty());
    auto second = queue.Advance(0.15F, false);
    assert(second.size() == 1 && second[0].ordinal == 2);
    assert(queue.Advance(1.0F, false).empty());
    queue.Add({ 3, 0.10F });
    queue.Reset(); // Pre-load/new-game removes delayed casts and recursion state.
    assert(queue.Advance(1.0F, false).empty());
    assert(queue.MarkSeen(100));
    queue.Forget(100); // A retired reference ID can be reused by a new projectile.
    assert(queue.MarkSeen(100));
    std::cout << "repeat queue regression: all assertions passed\n";
}
