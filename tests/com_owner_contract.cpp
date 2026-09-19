#include "kinoko/com_owner.hpp"
#include <cstdio>
#include <type_traits>
#include <utility>

namespace {
struct Reference {
    int refs = 1;
    int releases = 0;
    unsigned long Release() noexcept { ++releases; return --refs; }
    unsigned long AddRef() noexcept { return ++refs; }
};
using Owner = kinoko::ComOwner<Reference>;
static_assert(!std::is_copy_constructible_v<Owner>);
static_assert(!std::is_copy_assignable_v<Owner>);
static_assert(std::is_nothrow_move_constructible_v<Owner>);
static_assert(std::is_nothrow_move_assignable_v<Owner>);
#define CHECK(value) do { if (!(value)) { \
    std::fprintf(stderr, "owner contract line %d: %s\n", __LINE__, #value); return 1; \
} } while (0)
}
int main() {
    Reference first, second;
    {
        Owner a(&first);
        CHECK(a && a.get() == &first && first.refs == 1);
        a.reset(a.get());
        CHECK(first.releases == 0);
        auto b = std::move(a);
        CHECK(!a && b.get() == &first && first.releases == 0);
        b = std::move(b);
        CHECK(b.get() == &first && first.releases == 0);
        Owner c(&second);
        c = std::move(b);
        CHECK(!b && c.get() == &first && second.releases == 1);
        auto** out = c.put();
        CHECK(first.refs == 0 && first.releases == 1 && !c);
        Reference third;
        *out = &third;
        CHECK(c.get() == &third);
        auto* detached = c.detach();
        CHECK(!c && detached == &third && third.releases == 0);
        detached->Release();
        CHECK(third.releases == 1 && third.refs == 0);
    }
    CHECK(first.releases == 1 && second.releases == 1);
    Reference shared;
    {
        Owner a(&shared);
        // Explicitly creating a second owner requires a second reference.
        shared.AddRef();
        Owner b(&shared);
        a = std::move(b);
        CHECK(!b);
        CHECK(shared.refs == 1 && shared.releases == 1);
        a.reset();
        CHECK(shared.refs == 0 && shared.releases == 2);
    }
    std::puts("PASS: single-reference ownership, borrowing, move, detach and out-parameters");
}
