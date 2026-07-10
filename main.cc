#include "core/core.hpp"
#include "core/pool.hpp"
#include "core/allocator.hpp"
#include "core/memptr.hpp"
#include <cassert>

void inttests() {
    auto log_file = fmt::output_file("logs/inttests.log");
    auto pool = core::pool(core::hectobyte);
    core::i32 *x = nullptr;
    assert(pool.allocate<core::u8>(1));
    x = pool.allocate<core::i32>(1);
    while(pool.allocate<core::i32>(1));
    if(x) *x = 32;
    if(x) log_file.print("*x => {}\n", *x);
    if(!x) log_file.print("*x => (nil)\n");
}

void memptrtests() {
    auto log_file = fmt::output_file("logs/memptrtests.log");
    auto ints = core::alloc::byte<core::i32,core::kilobyte>();     
    auto x = core::memptr<core::i32>();
    x.allocate(ints);
    *x = 23;
    log_file.print("*x => {}\n", *x);
}

auto main() noexcept -> core::i32 {
    inttests();
    memptrtests();
}
