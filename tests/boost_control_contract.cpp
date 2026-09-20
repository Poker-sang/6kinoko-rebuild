#include "kinoko/boost_control.hpp"
#if defined(_WIN32)
#include <boost/smart_ptr/detail/sp_counted_base_w32.hpp>
#else
#include <boost/smart_ptr/detail/sp_counted_base_pt.hpp>
#endif
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <thread>
#include <vector>
namespace up = kinoko::native::upstream;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void owner_slot() {
    int actor = 1729;
    auto slot = static_cast<int**>(std::malloc(sizeof(int*)));
    require(slot != nullptr, "owner slot allocation");
    *slot = &actor;
    auto control = up::create_owner_control(slot);
    require(control && up::owns_control(control), "factory constructs an upstream-backed object");
    require(up::allocation(control) == slot && up::use_count(control) == 1, "initial count and owner slot");
    up::add_weak(control);
    require(up::lock(control) && up::use_count(control) == 2, "lock increments a nonzero count");
    up::release_strong(control);
    require(up::use_count(control) == 1, "nonfinal release");
    up::release_strong(control);
    require(up::use_count(control) == 0 && up::allocation(control) == nullptr && actor == 1729,
            "dispose clears/frees only the slot, not the borrowed Actor");
    require(!up::lock(control), "expired weak pointer cannot resurrect");
    up::release_weak(control);
    require(!up::owns_control(nullptr), "null is not an owned control");
}
// Exercise the genuine counted-base algorithm with an observable virtual
// deleter. This is not a substitute implementation of the reference counter.
struct Events : boost::detail::sp_counted_base {
    std::atomic<unsigned> disposed{0}, destroyed{0}, errors{0};
    void dispose() override {
        if (use_count() != 0 || destroyed != 0) ++errors;
        ++disposed;
    }
    void destroy() override {
        if (use_count() != 0 || disposed != 1) ++errors;
        ++destroyed;
        // Test fixture storage lives until the owning scope ends.
    }
    void* get_deleter(const boost::detail::sp_typeinfo&) override { return nullptr; }
};
void race(bool final_release) {
    for (int round = 0; round < 32; ++round) {
        Events control;
        control.weak_add_ref();
        std::atomic<bool> start{false};
        std::atomic<unsigned> failures{0}, successes{0};
        std::vector<std::thread> workers;
        for (int i = 0; i < 4; ++i) workers.emplace_back([&] {
            while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
            for (int i = 0; i < 1024; ++i) {
                if (control.add_ref_lock()) { ++successes; control.release(); }
                else if (!final_release) ++failures;
            }
        });
        start.store(true, std::memory_order_release);
        if (final_release) control.release();
        for (auto& worker : workers) worker.join();
        if (!final_release) {
            require(successes == 4096 && control.use_count() == 1 && control.disposed == 0,
                    "surviving owner permits all contended locks");
            control.release();
        }
        require(failures == 0 && control.errors == 0 && control.disposed == 1 && control.destroyed == 0,
                "last strong reference disposes once; weak reference retains control");
        require(!control.add_ref_lock(), "zero is terminal after the race");
        control.weak_release();
        require(control.destroyed == 1 && control.errors == 0, "last weak reference destroys after dispose");
    }
}
}
int main() {
    try {
        owner_slot(); race(false); race(true);
        std::puts("Boost 1.44 counted-base: slot ownership and 262144 contended weak locks passed");
    } catch (const std::exception& e) { std::fprintf(stderr, "%s\n", e.what()); return 1; }
}
