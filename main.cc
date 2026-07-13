#include "core/core.hpp"
#include "core/pool.hpp"
#include "core/allocator.hpp"
#include "core/memptr.hpp"
#include "core/defer.hpp"
#include <cassert>
#include <cstring>

#define FMTLOG(X) do {\
    fmt::print("{:?}:\n\t{} => {}\n", __PRETTY_FUNCTION__, #X, X);\
} while(false)

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

namespace dev {

template<typename char_t_ = char>
struct string {
    using char_t = char_t_;
    core::memptr<char_t> underlying;
    
    [[nodiscard]] constexpr auto
    length() noexcept {
        return std::strlen(underlying.get());
    }

    template<typename Alloc> constexpr decltype(auto)
    append(Alloc &&alloc, const char_t *const other) noexcept {
        if(underlying.extent == 0) {
            underlying.allocate(alloc, core::hexabyte);
            std::memset(underlying.get(), '\0', underlying.extent);
        }
        auto self_len = length();
        auto other_len = std::strlen(other);
        
        auto space_to_realloc = underlying.extent;
        while(self_len + other_len + 1 >= space_to_realloc) {
            space_to_realloc *= 2;
        }

        if(space_to_realloc != underlying.extent) {
            underlying.reallocate(std::forward<Alloc>(alloc), space_to_realloc);
            assert(underlying.extent >= self_len); // sanity and bad math check
            std::memset(underlying.get() + self_len, '\0', underlying.extent - self_len);
        }

        if(self_len + other_len + 1 < underlying.extent) {
            std::memcpy(underlying.get() + self_len, other, other_len);
        }
        return *this;
    }

    template<typename Alloc> constexpr decltype(auto)
    append(Alloc &&alloc, string<> other) noexcept {
        return append(std::forward<Alloc>(alloc), other.underlying.get());
    }

    template<typename Alloc> constexpr decltype(auto) append(Alloc &&alloc, char_t ch) noexcept {
        char_t buffer[] = { ch, 0 };
        return append(std::forward<Alloc>(alloc), buffer);
    }
};

};

void devstrtest() {
    auto logfile = fmt::output_file("logs/devstrtest.log");

    //auto chars = core::alloc::byte<char,core::kilobyte>();
    auto chars = std::allocator<char>();
    dev::string<> name;
    std::memset(name.underlying.get(), '\0', name.underlying.extent);
    name.append(chars, "CARTER__");
    name.append(chars, "CARTER__");

    logfile.print("name => {}\n", name.underlying.get());
    logfile.print("\tlength() => {}\n\tcapacity => {}\n",
        name.length(),
        name.underlying.extent
    );

    logfile.print("bytes:\n");
    for(core::size i = 0; i < name.underlying.extent; ++i) {
        logfile.print("\t[{}] => {:?}\n", i, name.underlying[i]);
    }

    name.underlying.deallocate(chars);

    core::memptr<dev::string<>> name2;
    name2.allocate(std::allocator<dev::string<>>());
    core::defer l0 ([&]{ name2.deallocate(std::allocator<dev::string<>>()); });

    std::construct_at(name2.get());
    core::defer l1 ([&]{ std::destroy_at(name2.get()); });
    
    core::defer l2 ([&]{ name2->underlying.deallocate(chars); });
    name2->append(chars, "CARTE@__");
    name2->append(chars, "auto&&_[[]]=[=][[]]<>([[]]){}();");
    name2->append(chars, "CARTE@__");
    name2->append(chars, "CARTE@__");

    logfile.print("name2 => {}\n", name2->underlying.get());
}

void devstrtest2() {
    auto pool = core::pool<core::alloc::byte<char,core::kilobyte>>(core::kilobyte);
    auto alloc = pool.adapt<char>();
    dev::string<> x;
    dev::string<> y;
    x.append(alloc, 'A');
    x.append(alloc, 'B');
    x.append(alloc, 'C');
    x.append(alloc, 'D');
    x.append(alloc, 'E');
    y.append(alloc, "FGHIJ");

    x.append(alloc, y);
    x.append(alloc, y);
    x.append(alloc, y);
    x.append(alloc, y);

    fmt::output_file("logs/devstrtest2.log").print("x => {}\n", x.underlying.get());
}

auto main() noexcept -> core::i32 {
    devstrtest();
    inttests();
    memptrtests();
    devstrtest2();
}
