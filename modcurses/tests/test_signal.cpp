#include <string>
#include <vector>

#include "doctest.h"
#include "modcurses/core.hpp"

using namespace modcurses;

TEST_CASE("connect, emit, slot_count") {
    Signal<int> sig;
    int seen = 0;
    CHECK(sig.slot_count() == 0);

    auto c = sig.connect([&](int v) { seen += v; });
    CHECK(sig.slot_count() == 1);
    CHECK(c.connected());

    sig.emit(3);
    sig(4);  // operator() is the same thing
    CHECK(seen == 7);

    c.disconnect();
    CHECK_FALSE(c.connected());
    CHECK(sig.slot_count() == 0);
    sig.emit(100);
    CHECK(seen == 7);
}

TEST_CASE("slots are called in connection order") {
    Signal<> sig;
    std::string order;
    auto a = sig.connect([&] { order += "a"; });
    auto b = sig.connect([&] { order += "b"; });
    auto c = sig.connect([&] { order += "c"; });
    sig.emit();
    CHECK(order == "abc");
}

TEST_CASE("a slot connected during an emit is not called by that emit") {
    Signal<> sig;
    int inner_calls = 0;
    Connection inner;
    auto outer = sig.connect([&] { inner = sig.connect([&] { ++inner_calls; }); });

    sig.emit();
    CHECK(inner_calls == 0);  // added mid-emit: skipped this time round
    CHECK(sig.slot_count() == 2);

    sig.emit();
    CHECK(inner_calls == 1);
}

TEST_CASE("a slot disconnected during an emit does not fire") {
    Signal<> sig;
    int a_calls = 0, b_calls = 0;
    Connection b;
    auto a = sig.connect([&] {
        ++a_calls;
        b.disconnect();  // b has not run yet this emit
    });
    b = sig.connect([&] { ++b_calls; });

    sig.emit();
    CHECK(a_calls == 1);
    CHECK(b_calls == 0);
    CHECK(sig.slot_count() == 1);  // tombstone compacted after the emit
}

TEST_CASE("a slot may disconnect itself mid-emit") {
    Signal<> sig;
    int calls = 0;
    Connection self;
    self = sig.connect([&] {
        ++calls;
        self.disconnect();
    });
    sig.emit();
    sig.emit();
    CHECK(calls == 1);
    CHECK(sig.slot_count() == 0);
}

TEST_CASE("compaction is deferred until the OUTERMOST emit returns") {
    Signal<> sig;
    std::vector<int> hits;
    Connection first, second;
    first = sig.connect([&] {
        hits.push_back(1);
        first.disconnect();   // also stops the nested emit recursing forever
        second.disconnect();
        sig.emit();           // re-entrant: must not compact under the outer walk
    });
    second = sig.connect([&] { hits.push_back(2); });
    auto third = sig.connect([&] { hits.push_back(3); });

    sig.emit();
    // Slot 2 never runs: tombstoned before either walk reached it. Slot 3 runs
    // once per walk. If the inner emit had compacted the vector, the outer
    // walk's indices would have slid and slot 3 would have been skipped or
    // double-counted differently.
    CHECK(hits == std::vector<int>{1, 3, 3});
    CHECK(sig.slot_count() == 1);
    CHECK(third.connected());
}

TEST_CASE("connections outlive their signal without dangling") {
    Connection c;
    {
        Signal<> sig;
        c = sig.connect([] {});
        CHECK(c.connected());
    }
    CHECK_FALSE(c.connected());
    c.disconnect();  // must be a safe no-op, not a use-after-free
}

TEST_CASE("ScopedConnection disconnects on destruction") {
    Signal<> sig;
    int calls = 0;
    {
        ScopedConnection sc = sig.connect([&] { ++calls; });
        sig.emit();
        CHECK(calls == 1);
        CHECK(sc.connected());
    }
    CHECK(sig.slot_count() == 0);
    sig.emit();
    CHECK(calls == 1);
}

TEST_CASE("ScopedConnection move transfers ownership exactly once") {
    Signal<> sig;
    int calls = 0;
    {
        ScopedConnection outer;
        {
            ScopedConnection inner = sig.connect([&] { ++calls; });
            outer = std::move(inner);
        }
        sig.emit();  // inner is gone, but outer holds the connection
        CHECK(calls == 1);
    }
    sig.emit();
    CHECK(calls == 1);
}

TEST_CASE("moving a Signal keeps existing connections alive and pointing at it") {
    Signal<int> a;
    int seen = 0;
    auto c = a.connect([&](int v) { seen = v; });

    Signal<int> b{std::move(a)};
    CHECK(c.connected());
    b.emit(42);
    CHECK(seen == 42);

    c.disconnect();  // must reach b, the new owner
    CHECK(b.slot_count() == 0);
}

TEST_CASE("multiple arguments are forwarded to every slot") {
    Signal<int, std::string> sig;
    int sum = 0;
    std::string cat;
    auto c1 = sig.connect([&](int i, std::string s) { sum += i; cat += s; });
    auto c2 = sig.connect([&](int i, std::string s) { sum += i * 10; cat += s; });
    sig.emit(2, "x");
    CHECK(sum == 22);
    CHECK(cat == "xx");
}

TEST_CASE("clear during an emit tombstones rather than invalidating the walk") {
    Signal<> sig;
    int calls = 0;
    auto a = sig.connect([&] { ++calls; sig.clear(); });
    auto b = sig.connect([&] { ++calls; });
    sig.emit();
    CHECK(calls == 1);  // b was tombstoned before it ran
    CHECK(sig.slot_count() == 0);
}
