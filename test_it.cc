#include "dynamic_array.hpp"
#include <string>
#include <functional>
#ifndef NDEBUG
#include <iostream>
#endif
#include <fstream>
#include <algorithm>
#include <ranges>
#include <numeric>
#include <cassert>
#include <cstdlib>
#include <bit>
#include <random>
#include <limits>
#include <memory>

#ifndef NDEBUG
constexpr bool DEBUG = true;
#else
constexpr bool DEBUG = false;
#endif

template<typename T>
requires std::is_trivially_copyable_v<T>
struct fuzzed {
    volatile unsigned char data[sizeof(T)];
    fuzzed() {
        std::random_device rd;  // a seed source for the random number engine
        std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> distrib(0, std::numeric_limits<unsigned char>::max());
        for(std::size_t i = 0; i < sizeof(T); ++i) {
            data[i] = static_cast<unsigned char>(distrib(rd));
        }
    }
    constexpr T get() {
        return std::bit_cast<T>(*this);
    }
    constexpr operator T() {
        return std::bit_cast<volatile T>(*this);
    }
};

struct lifetime {
    lifetime() {
#ifndef NDEBUG
        std::cout << __PRETTY_FUNCTION__ << '\n';
#endif
    }
    lifetime(lifetime&) {
#ifndef NDEBUG
        std::cout << __PRETTY_FUNCTION__ << '\n';
#endif
    }
    lifetime(lifetime&&) {
#ifndef NDEBUG
        std::cout << __PRETTY_FUNCTION__ << '\n';
#endif
    }
    lifetime& operator=(lifetime&) {
#ifndef NDEBUG
        std::cout << __PRETTY_FUNCTION__ << '\n';
#endif
        return *this;
    }
    lifetime& operator=(lifetime&&) {
#ifndef NDEBUG
        std::cout << __PRETTY_FUNCTION__ << '\n';
#endif
        return *this;
    }
    ~lifetime() {
#ifndef NDEBUG
        std::cout << __PRETTY_FUNCTION__ << '\n';
#endif
    }
};

#define always_assert(condition) do { always_assert_((condition), #condition); } while(false)

constexpr void
always_assert_(bool condition, [[maybe_unused]] std::string_view words) {
    if (!condition) {
#ifndef NDEBUG
        std :: cerr << words;
#endif
        exit(-1);
    }
}

void
access_test () {
#ifndef NDEBUG
    (std :: cout << __PRETTY_FUNCTION__) .put('\n');
#endif

    darray <int> nums;
    nums .deb (1) .deb (2) .deb (3);

    always_assert (nums[0] == 1);
    always_assert (nums[1] == 2);
    always_assert (nums[2] == 3);

    darray <int> nums2;
    nums2 .deb (1) .deb(25) .deb(1);

    [](darray <int> const & arr) {
        always_assert (arr [1] == 25);
    } (nums2);

    auto && test = [](darray <int> arr) {
        always_assert (arr [1] == 25);
    };
    
    test (nums2);
    test (std :: move (nums2));
}

void
const_test() {
#ifndef NDEBUG
    (std :: cout << __PRETTY_FUNCTION__) .put('\n');
#endif

    darray <int> nums (1, 2, 3);

    const int x = 11;
    nums.pb(x);

    [](darray<int> const arr) {
        always_assert(arr[3] == 11);
    }(nums);
}

void
arena_test() {
    auto alloc = core::arena(256);
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

    std::destroy_at(y);
    std::destroy_at(x);
    std::destroy_at(z);
}

[[nodiscard]] consteval bool
arena_ub_test() {
    auto alloc = core::stack_typed_allocator<int,256>();
    decltype(auto) x = alloc.allocate(1);
    std::construct_at(x, 23);
    assert(*x == 23);
    std::destroy_at(x);
    return true;
}

void
arena_test2() {
    static constexpr std::size_t CAPACITY(1024);
    static auto storage = core::arena_loud<core::stack_byte_allocator<std::byte,CAPACITY>>(CAPACITY, "fibbo_cache_out.txt");
    storage.reset();

    static auto fib = std::function<int(int)>(nullptr);
    constexpr auto results_size = decltype(storage)::size_type(32);
    int **results = storage.allocate<int*>(results_size);
    if(results) std::for_each(results, results + results_size,
        [](int *&to_null) { to_null = nullptr; }
    );

    if(results) results[0] = storage.allocate<int>(1);
    if(results && results[0]) { *results[0] = 0; }
    if(results) results[1] = storage.allocate<int>(1);
    if(results && results[1]) { *results[1] = 1; }

    fib = [&](int N) -> int {
        if(static_cast<std::size_t>(N) >= CAPACITY) return std::numeric_limits<decltype(N)>::max(); // infinity!
        if(N == 0) return 0;
        if(N == 1) return 1;
        if(!results) return fib(N - 1) + fib(N - 2);
        if(!results[N]) {
            results[N] = storage.allocate<int>(1);
            if(!results[N]) return fib(N - 1) + fib(N - 2);
            *results[N] = fib(N - 1) + fib(N - 2);
        }
        return *results[N];
    };
    

    decltype(auto) fout = std::ofstream("out.txt");
    if constexpr(DEBUG) {
        std::ranges::for_each(
            std::ranges::views::iota(0, static_cast<int>(results_size)),
            [&](int i) {
                fout << i << " => " << fib(i) << '\n';
            }
        );
    }

    fout << "total bytes used: " << static_cast<decltype(storage)::size_type>(
        storage.current - storage.payload
    );
}

template<typename T>
using no_op_t = decltype([](T){});

void arena_loud_test() {
    auto arena1 = core::arena_loud<core::stack_byte_allocator<std::byte,1024>>(1024, "arena_loud_out.txt");
    auto arena2 = core::arena_loud<std::allocator<std::byte>>(1024, "arena_loud_out.txt");
    
    std::unique_ptr<std::string,no_op_t<std::string*>> ptr;

    ptr.reset(arena1.allocate<std::string>(1));
    std::construct_at(ptr.get(), "Carter Aitken");
    assert(*ptr == "Carter Aitken");
    std::destroy_at(ptr.get());

    arena1.reset();
    ptr.reset(arena1.allocate<std::string>(1));
    new(ptr.get()) std::string("Carter Aitken2");
    ptr->operator+=("[ADDON!]");

    fmt::ostream out = fmt::output_file("tests/arena_loud.out");
    out.print("{}\n", *ptr);
    out.close();

    core::run_test("arena_loud");
    
    std::destroy_at(ptr.get());
}

void leak_test() {
    auto alloc = std::allocator<std::string>();
    decltype(auto) s = alloc.allocate(1);
    std::construct_at(s, "Carter");
    decltype(auto) sview = std::string_view(*s);

    fmt::ostream out = fmt::output_file("tests/leak.out");
    out.print("{}\n", sview);
    out.close();

    core::run_test("leak");

    std::destroy_at(s);
    alloc.deallocate(s, 1);
}

void
string_test() {
    using core_string = std::basic_string<char,std::char_traits<char>, core::stack_byte_resetting_allocator<char,64>>;
    auto str0 = core_string("Carter Aitken");
    for(auto i [[maybe_unused]] : std::ranges::views::iota(0zu,10zu)) {
        if(str0.capacity() * 2 > 64) break;
        constexpr static std::string_view nums = "1234567890";
        str0 += nums;
    }

    auto alloc = core::arena_loud<core::stack_byte_allocator<std::byte,1024>>(1024, "logs/string_test_allocator.log");
    decltype(auto) str1 = alloc.allocate<core_string>(1);
    std::construct_at(str1, "Carter Aitken");
    *str1 += "[SUPRISE!]";
    
    fmt::output_file("tests/custom_string.out").print("{:?}\n", *str1);
    core::run_test("custom_string");

    std::destroy_at(str1);

}

void
general_usage() {
    auto memory = core::arena<core::stack_byte_allocator<std::byte,core::kilobyte>>(core::kilobyte);
    auto name = std::unique_ptr<core::capstr<core::decabyte>,no_op_t<core::capstr<core::decabyte>*>>();
    name.reset(memory.allocate<core::capstr<core::decabyte>>(1));
    std::construct_at(name.get(), "Carter Aitken");
    name->operator+=("[[]][=][[]]->int[][[]]{} is valid cpp");
    fmt::output_file("tests/general_usage.out").print("{:?}\n", *name);
    std::destroy_at(name.get());

    auto name2 = core::capstr<core::hectobyte>("Carter Aitken");

    core::run_test("general_usage");
}

int main() {
    access_test();
    const_test();

    arena_test();
    static_assert(arena_ub_test());
    arena_test2();
    arena_test2();
    arena_test2();
    arena_loud_test();
    leak_test();

    string_test();

    general_usage();

    return EXIT_SUCCESS;
}
