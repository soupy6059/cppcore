#include "core/core.hpp"
#include "core/pool.hpp"
#include "core/allocator.hpp"
#include "core/memptr.hpp"
#include "core/defer.hpp"
#include <cassert>
#include <cstring>
#include <ranges>
#include <algorithm>
#include <expected>
#include <source_location>

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

namespace dev {

template<typename T>
struct safeptr {
    constexpr decltype(auto) allocate(auto &&alloc, core::size amount) noexcept {
        payload = std::forward<decltype(alloc)>(alloc).allocate(amount);
        extent = payload? amount : 0;
        return *this;
    }

    constexpr decltype(auto) reallocate(auto &&alloc, core::size amount, auto &&...args) noexcept {
        safeptr nextptr;
        nextptr
            .allocate(alloc, amount)
            .construct_each(std::forward<decltype(args)>(args)...);
        for(core::size i = 0; i < std::min(nextptr.extent, extent); ++i) {
            nextptr.payload[i] = std::move(payload[i]); // memcpy instead?
        }
        
        destroy_each().deallocate(std::forward<decltype(alloc)>(alloc));

        std::swap(nextptr, *this);

        return *this;
    }

    constexpr decltype(auto) deallocate(auto &&alloc) noexcept {
        assert(!constructed);
        std::forward<decltype(alloc)>(alloc).deallocate(payload, extent);
        payload = nullptr;
        extent = 0;
        constructed = false;
        return *this;
    }

    constexpr decltype(auto) construct_each(auto &&...args) noexcept {
        for(core::size i = 0; i < extent; ++i) {
            std::construct_at(payload + i, std::forward<decltype(args)>(args)...);
        }
        if(payload) constructed = true;
        return *this;
    }

    constexpr decltype(auto) destroy_each() noexcept {
        for(core::size i = 0; i < extent; ++i) {
            std::destroy_at(payload + i);
        }
        constructed = false;
        return *this;
    }

    constexpr decltype(auto) for_each(auto &&callable) & noexcept {
        for(core::size i = 0; i < extent; ++i) {
            callable(payload[i]);
        }
        return *this;
    }

    constexpr decltype(auto) for_each(auto &&callable) && noexcept {
        assert(constructed);
        for(core::size i = 0; i < extent; ++i) {
            callable(std::move(payload[i]));
        }
        return *this;
    }

    constexpr decltype(auto) for_at(core::size i, auto &&callable) noexcept {
        if(i < extent) {
            assert(constructed);
            std::forward<decltype(callable)>(callable)(payload[i]);
        }
        return *this;
    }

    constexpr decltype(auto) map(auto &&callable) noexcept {
        for(core::size i = 0; i < extent; ++i) {
            payload[i] = callable(payload[i]);
        }
        return *this;
    }

    template<typename U>
    constexpr safeptr<U> transform(auto &&alloc_for_t, auto &&alloc_for_u, auto &&callable) noexcept {
        safeptr<U> nextptr;
        nextptr.allocate(std::forward<decltype(alloc_for_u)>(alloc_for_u), extent);
        nextptr.construct_each();

        core::size i = 0;
        nextptr.map([&](U&) { return callable(payload[i++]); });

        destroy_each();
        deallocate(std::forward<decltype(alloc_for_t)>(alloc_for_t));

        return nextptr;
    }

    void print_memory() {
        fmt::print("extent = {}, constructed = {}, payload = {}\n", extent, constructed, static_cast<void*>(payload));
    }

private:
    T *payload = nullptr;
    core::size extent = 0;
    bool constructed = false;
};

};

namespace dev {

template<typename T>
struct errptr {
    enum class error {
        allocation_failure,
    };

    errptr &operator=(errptr const &other) = delete;
    errptr(errptr const &other) = delete;

    [[nodiscard]] constexpr std::expected<errptr,error> allocate(auto &&alloc, core::size amount) noexcept {
        payload = std::forward<decltype(alloc)>(alloc).allocate(amount);
        if(!payload) {
            return std::unexpected(error::allocation_failure);
        }
        extent = payload? amount : 0;
        return std::move(*this);
    }

private:
    core::size extent;
    bool constructed = false;
    T *payload = nullptr;
};

};

auto devsafeptr1() {
    auto logfile = fmt::output_file("logs/devsafeptr1.log");
    auto alloc = std::allocator<core::i32>();
    auto alloc_d = std::allocator<double>();
    dev::safeptr<core::i32>()
        .allocate(alloc, 5)
        .construct_each()
        .map([](auto){ return 32; })
        .reallocate(alloc, 10, 45)
        .transform<double>(alloc, alloc_d, [](auto x) { return static_cast<double>(x) + 0.2332; })
        .for_each([&logfile](auto &&x) { logfile.print("{}\n", x); })
        .destroy_each()
        .deallocate(alloc_d);
}

// migrate?

auto pooltesting() {
    auto logfile = fmt::output_file("logs/pooltesting.log");
    auto pool = core::pool<core::alloc::byte<std::byte,core::kilobyte>>(core::kilobyte);
    char *excl = pool.allocate<char>(1);
    *excl = '!';

    dev::safeptr<std::string>()
    .allocate(pool.adapt<std::string>(), 1)
    .construct_each("Carter Aitken")
    .for_each([excl](std::string &x) { x += excl; })
    .for_each([&logfile](std::string &x) {
        logfile.print("my name is {}\n", x);
    })
    .destroy_each();

    dev::safeptr<dev::string<>>()
    .allocate(pool.adapt<dev::string<>>(), 1)
    .construct_each()
    .for_each([&pool](dev::string<> &s) {
        s.append(pool.adapt<char>(), "Carter Aitken!");
    })
    .for_each([&logfile](dev::string<> &x) {
        logfile.print("[2] my name is {}\n", x.underlying.payload);
    })
    .destroy_each();
}

void
pooltest() {
    auto alloc = core::pool(core::kilobyte);
    decltype(auto) x = alloc.allocate<int>(1);
    decltype(auto) y = alloc.allocate<int>(1);
    decltype(auto) z = alloc.allocate<std::string>(1);

    std::construct_at(x, 23);
    std::construct_at(y, 34);
    std::construct_at(z, "[CA]");

    assert(*x == 23);
    assert(*y == 34);
    assert(*x != *y);

    assert(*z == "[CA]");

    alloc.print_memory();

    std::destroy_at(y);
    std::destroy_at(x);
    std::destroy_at(z);
}

#if 0
[[nodiscard]] constexpr auto
fib(core::i32 x, dev::safeptr<dev::safeptr<core::i32>> &cache, auto &&pool) noexcept -> core::i32 {
    assert(x >= 0);
    if(x == 0) return 0;
    if(x == 1) return 1;
    core::i32 to_return = -1;
    cache.for_at(static_cast<core::size>(x), [&](dev::safeptr<core::i32> &previous_result_box) {
        previous_result_box.for_at(0, [&](core::i32 previous_result) {
            to_return = previous_result;
        });
    });
    if(to_return != -1) return to_return; 

    to_return = fib(x - 1, cache, pool) + fib(x - 2, cache, pool);
    cache.for_at(static_cast<core::size>(x), [to_return,&pool](dev::safeptr<core::i32> &nulled) {
        nulled.allocate(pool. template adapt<core::i32>(), 1)
            .construct_each(to_return);
    });
    return to_return;
}

void
pool_test2() {
    std::puts("=============== START HERE ====================\n\n\n\n");
    std::puts(__PRETTY_FUNCTION__);
    static constexpr core::size memory_capacity = core::hectobyte * 3;
    static auto storage = core::pool<core::alloc::byte<std::byte,memory_capacity>>(memory_capacity);
    storage.reset();
    
    core::i32 cache_size = 16;
    auto cache = dev::safeptr<dev::safeptr<core::i32>>()
        .allocate(storage.adapt<dev::safeptr<core::i32>>(), static_cast<core::size>(cache_size))
        .construct_each();

    // storage.print_memory();

    cache.for_at(0, [&](dev::safeptr<core::i32> &ptr) {
        ptr.allocate(storage.adapt<core::i32>(), 1)
            .construct_each(0);
    });

    cache.for_at(1, [&](dev::safeptr<core::i32> &ptr) {
        ptr.allocate(storage.adapt<core::i32>(), 1)
            .construct_each(1);
    });
    
    auto compute_cache [[maybe_unused]] = fib(cache_size, cache, storage);
    storage.print_memory();

    auto logfile = fmt::output_file("logs/pool_test2.log");
    std::ranges::for_each(
        std::ranges::views::iota(0, cache_size),
        [&](core::i32 i) {
            logfile.print("{} => {}\n", i, fib(i, cache, storage));
        }
    );
}

#endif

core::i32 fib2(core::i32 x) {
    static auto storage = core::pool<std::allocator<std::byte>> (core::kilobyte);
    static core::i32 *cache[core::hectobyte] = {nullptr};
    storage.print_memory();
    if(not (0 <= x and static_cast<core::size>(x) < core::hectobyte)) {
        return -1;
    }

    // base case
    {
        if(x == 0) return 0;
        if(x == 1) return 1;
    }

    if(cache[x]) return *cache[x];

    core::i32 answer = fib2(x - 1) + fib2(x - 2);

    if((cache[x] = storage.allocate<core::i32>(1))) {
        *cache[x] = answer;
    }

    return answer;
}

core::i32 fib3(core::i32 x) {
    static auto storage = core::pool<core::alloc::byte<std::byte,core::kilobyte>> (core::kilobyte);
    static core::i32 *cache[core::hectobyte] = {nullptr};
    storage.print_memory();
    if(not (0 <= x and static_cast<core::size>(x) < core::hectobyte)) {
        return -1;
    }

    // base case
    {
        if(x == 0) return 0;
        if(x == 1) return 1;
    }

    if(cache[x]) return *cache[x];

    core::i32 answer = fib2(x - 1) + fib2(x - 2);

    if(( cache[x] = storage.allocate<core::i32>(1) )) {
        *cache[x] = answer;
    }

    return answer;
}

core::i32 fib4(core::i32 x) {
    static auto storage = core::pool<core::alloc::byte<std::byte,core::kilobyte>> (core::kilobyte);
    static dev::safeptr<core::i32> cache[core::hectobyte];
    storage.print_memory();
    if(not (0 <= x and static_cast<core::size>(x) < core::hectobyte)) {
        return -1;
    }

    // base case
    {
        if(x == 0) return 0;
        if(x == 1) return 1;
    }
    
    core::i32 getter = -1;
    cache[x].for_each([&](int answer) { getter = answer; });
    if(getter != -1) return getter;
    
    auto answer = fib4(x - 1) + fib4(x - 2);
    cache[x].allocate(storage.adapt<core::i32>(), 1)
        .construct_each(answer);

    return answer;
}


// storage => 4 kilobytes
static auto storage = core::pool<core::alloc::byte<std::byte,4 * core::kilobyte>> (4 * core::kilobyte);
core::i32 fib5(core::i32 x) {
    static dev::safeptr<dev::safeptr<core::i32>> cache;
    static core::size call_count = 0;
    if(call_count == 0) {
        // cache => 60 dev::safeptr<core::i32> @ storage
        cache.allocate(storage.adapt<dev::safeptr<core::i32>>(), 60).construct_each();
    }
    ++call_count;

    if(not (0 <= x and static_cast<core::size>(x) < core::hectobyte)) {
        return -1;
    }

    // base case
    {
        if(x == 0) return 0;
        if(x == 1) return 1;
    }
    
    core::i32 getter = -1;
    cache.for_at(static_cast<core::size>(x), [&](dev::safeptr<core::i32> &cache_local) {
        cache_local.for_each([&](core::i32 answer) { getter = answer; });
    });
    if(getter != -1) return getter;

    auto answer = fib5(x - 1) + fib5(x - 2);
    cache.for_at(static_cast<core::size>(x), [&](dev::safeptr<core::i32> &cache_local) {
        // cache_local => 1 core::i32 @ storage + cache
        cache_local.allocate(storage.adapt<core::i32>(), 1)
        .construct_each(answer);
    });

    return answer;
}

namespace dev {

template<typename T>
struct verbose_ptr {
private:
#define set_error(type, error) (print_error_set_(#type, #error), type :: error)
    void print_error_set_(std::string_view type, std::string_view error, std::source_location location = std::source_location::current()) {
        os.print("{}@{}; {}({}:{}) `{}`\n", type.data(), error.data(), location.file_name(), location.line(), location.column(), location.function_name());
        os.flush();
    }
public:

    static constexpr verbose_ptr make(std::string_view filename, std::string_view message, std::source_location location = std::source_location::current()) noexcept {
        verbose_ptr ptr(nullptr, 0, false, filename);
        ptr.print_self(message, location);
        ptr.print_self("after default construction of the pointer", location);
        return ptr;
    }
    
    enum class allocate_error {
        NIL, NULLPTR_ON_ALLOCATION,
    };
    constexpr decltype(auto) allocate(auto &&alloc, core::size amount, std::source_location location = std::source_location::current()) noexcept {
        print_self("before allocation", location);
        auto error = allocate_error::NIL;
        payload = std::forward<decltype(alloc)>(alloc).allocate(amount);
        if(!payload) {
            error = set_error(allocate_error, NULLPTR_ON_ALLOCATION);
        }
        extent = payload? amount : 0;
        print_self("after allocation", location);
        return error;
    }

    constexpr decltype(auto) deallocate(auto &&alloc, std::source_location location = std::source_location::current()) noexcept {
        assert(!constructed);
        print_self("before deallocation", location);
        std::forward<decltype(alloc)>(alloc).deallocate(payload, extent);
        payload = nullptr;
        extent = 0;
        constructed = false;
        print_self("after deallocation", location);
        return *this;
    }

    template<typename tuple_t>
    constexpr decltype(auto) construct_each(tuple_t &&args, std::source_location location = std::source_location::current()) noexcept {
        print_self("before construction", location);
        if(extent) assert(payload);
        if(payload) assert(extent);
        assert(!constructed);
        for(core::size i = 0; i < extent; ++i) {
            std::apply(
                []<typename pointer_t, typename ...args_t>(pointer_t &&ptr, args_t &&...args_){
                    std::construct_at(std::forward<pointer_t>(ptr), std::forward<args_t>(args_)...);
                },
                std::tuple_cat(std::make_tuple(payload + i), std::forward<tuple_t>(args))
            );
        }
        if(payload) constructed = true;
        print_self("after construction", location);
        return *this;
    }

    enum class destroy_each_error {
        NIL,
        NO_PAYLOAD,
    };
    constexpr destroy_each_error destroy_each(std::source_location location = std::source_location::current()) noexcept {
        print_self("before destruction", location);
        if(extent) assert(payload);
        if(payload) assert(extent);
        assert(constructed);
        auto error = destroy_each_error::NIL;
        if(!payload) {
            error = set_error(destroy_each_error, NO_PAYLOAD);
        }
        for(core::size i = 0; i < extent; ++i) {
            std::destroy_at(payload + i);
        }
        constructed = false;
        print_self("after destruction", location);
        return error;
    }
    
    enum class for_each_error {
        NIL,
        NO_PAYLOAD,
    };
    constexpr for_each_error for_each(auto &&callable, std::source_location location = std::source_location::current()) {
        print_self("before for_each", location);
        if(extent) assert(payload);
        if(payload) assert(extent);
        auto error = for_each_error::NIL;
        if(!payload) { error = set_error(for_each_error, NO_PAYLOAD); }
        for(core::size i = 0; i < extent; ++i) {
            std::forward<decltype(callable)>(callable)(payload[i]);
        }
        print_self("after for_each", location);
        return error;
    }

private:
    void print_self(std::string_view message, std::source_location location) {
#if 0
        os.print("{}({}:{}) `{}`: {} => ", location.file_name(), location.line(), location.column(), location.function_name(), message.data());
        os.print("payload = {}, extent = {}, constructed = {}\n", reinterpret_cast<void*>(payload), extent, constructed);
#else
        os.print("{}({}:{}) `{}`: {} => ", location.file_name(), location.line(), location.column(), location.function_name(), message.data());
        os.print("[{}, {}, {}]\n", reinterpret_cast<void*>(payload), extent, constructed);
#endif
        os.flush();
    }
    
    verbose_ptr(T *payload_, core::size extent_, bool constructed_, std::string_view filename): payload(payload_), extent(extent_), constructed(constructed_), os(fmt::output_file(filename.data())) {}

    verbose_ptr(const verbose_ptr&) = default;
    verbose_ptr(verbose_ptr&&) = default;


    T *payload;
    core::size extent;
    bool constructed;
    fmt::ostream os;
};

};

void vptr_test() {
    struct bad_allocator {
        core::i32 *allocate(core::size) { return nullptr; }
        void deallocate(core::i32*,core::size) {}
    };
    auto alloc = bad_allocator();
    auto num = dev::verbose_ptr<core::i32>::make("logs/verpose_ptr_test.log", "making a verbose pointer");
    num.allocate(alloc, 1);
    num.construct_each(std::make_tuple(32));
    num.destroy_each();
    num.deallocate(alloc);
}

auto main() noexcept -> core::i32 {
    devstrtest();
    inttests();
    memptrtests();
    devstrtest2();
    devsafeptr1();
    pooltesting();
    pooltest();
#if 0
    pool_test2();
#endif
    vptr_test();
}
