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

FUCKCUFKCUJFI

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
    static auto storage = core::arena_loud<core::stack_byte_allocator<std::byte,CAPACITY>>(CAPACITY, "tests/fibbo_cache_out.out");
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
    

    decltype(auto) fout = std::ofstream("tests/nothing.out");
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
    auto arena1 = core::arena_loud<core::stack_byte_allocator<std::byte,1024>>(1024, "tests/arena_loud0.out");
    
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

void
strings() {
    auto memory = core::stack_byte_allocator<char,core::decabyte>();
//  auto memory = std::allocator<char>();
    core::string name;
    name.allocate(memory);
    name
    .append(memory, std::string_view("Carter__"))
    .append(memory, std::string_view("Carter__"))
    .append(memory, std::string_view("Carter__"))
    .append(memory, std::string_view("Carter__"))
    .append(memory, std::string_view("Carter__"))
    .append(memory, std::string_view("Carter__"))
    .append(memory, std::string_view("Carter__"))
    .append(memory, std::string_view("Carter__"))
    .append(memory, std::string_view("Carter__"))
    .append(memory, std::string_view("Carter__"))
    .append(memory, std::string_view("Carter__"))
    .append(memory, std::string_view("Carter_" ));
    assert(name);
    fmt::print("{:?}, size = {}, capacity = {}\n", name.c_str(), name.size(), name.capacity());
    name.deallocate(memory);

    auto nofree_memory = core::arena_loud<core::stack_byte_allocator<std::byte,core::kilobyte>>(core::kilobyte, "logs/nofree_memory.log");
    auto char_arena = nofree_memory.adapt<char>();
    core::string words;
    // Here note we haven't allocated our string yet, but since append
    // is passed an allocator, it's reasonable to assume it can allocate an
    // initial buffer.
    if(!words.append(char_arena, std::string_view("int main(void) { return EXIT_SUCCESS; }"))) {
        assert(false && "Bad Alloc!");
    }
    fmt::print("words == {:?}\n", words.c_str());

    std::ranges::for_each(
        std::ranges::views::iota(core::size(0), core::kilobyte),
        [&words, &char_arena](core::size i [[maybe_unused]]) {
            words.append(char_arena, std::string_view("."));
        }
    );
    assert(words && "shortting seems to be working!");
    fmt::print("word.size() == {}; word.capacity() == {}\n", words.size(), words.capacity());
    // should be about 64 bytes left in char_arena
    int *num = nofree_memory.allocate<int>(core::size(1));
    *num = 32;
    fmt::print("look! i got a num! -> {} <- !!! yay!\n", *num);
}

void
fuzzing_strings() {
    decltype(auto) alloc = std::allocator<char>();
    decltype(auto) to_fuzz = core::string();
    to_fuzz.allocate(alloc, core::decabyte);
    to_fuzz.append(alloc, std::string_view("Carter"));

    std::ranges::for_each(
        std::ranges::views::iota(core::size(0), core::kilobyte),
        [&](core::size i [[maybe_unused]]) {
            unsigned char to_append = (::fuzzed<unsigned char>().get() % 26) + 'A';
            to_fuzz.append(alloc, static_cast<char>(to_append));
        }
    );

    fmt::output_file("logs/fuzzed_string.log")
    .print("{1}/{2} -> {0}\n", to_fuzz.c_str(), to_fuzz.size(), to_fuzz.capacity());
    to_fuzz.deallocate(alloc);
}

template<typename Ptr, typename StringAllocator, typename CharAllocator>
void memptr_tests(std::string_view filename) {
    auto str_storage = StringAllocator();
    auto char_storage = CharAllocator();
    auto name = Ptr();
    name.allocate(str_storage);
    std::construct_at(name.payload);
    name.value().allocate(char_storage, core::hexabyte);
    
    name.value().append(char_storage, std::string_view("Carter Aitken!"));

    fmt::output_file(filename.data()).print("memptr_tests() => {}\n", name.value().c_str());
    
    name.value().deallocate(char_storage);
    std::destroy_at(name.payload);
    name.deallocate(str_storage);
}

void
matrix_test() {{
    auto alloc = std::allocator<double>();

    auto m0 = core::matrix<typename decltype(alloc)::value_type, 2, 2>();
    m0.underlying.allocate(alloc, m0.extent);
    std::uninitialized_value_construct_n(m0.underlying.payload, m0.extent);

    m0.at(0,0) = 2.0;
    m0.at(1,1) = 1.0;

    auto m1 = core::matrix<typename decltype(alloc)::value_type, 2, 3>();
    m1.underlying.allocate(alloc, m1.extent);
    std::uninitialized_value_construct_n(m1.underlying.payload, m1.extent);

    m1.at(0,0) = 2.0;
    m1.at(1,2) = 3.0;

    auto m2 = m0.multiply(alloc, m1);

    core::string<> m0_repr = m0.format(std::allocator<char>());
    fmt::print("m0 => {}\n", m0_repr.c_str());

    core::string<> m1_repr = m1.format(std::allocator<char>());
    fmt::print("m1 => {}\n", m1_repr.c_str());

    core::string<> m2_repr = m2.format(std::allocator<char>());
    fmt::print("m2 => {}\n", m2_repr.c_str());

    m0_repr.deallocate(std::allocator<char>());
    m1_repr.deallocate(std::allocator<char>());
    m2_repr.deallocate(std::allocator<char>());
    m0.underlying.deallocate(alloc);
    m1.underlying.deallocate(alloc);
    m2.underlying.deallocate(alloc);
} {
    auto alloc = core::arena<core::stack_byte_allocator<std::byte,core::kilobyte>>(core::kilobyte);
    using field = double;
    auto m0 = core::matrix<field, 2, 2>();
    m0.underlying.allocate(alloc.adapt<field>(), m0.extent);
    m0.at(0,0) = 3.0;
    m0.at(0,1) = 4.0;
    m0.at(1,0) = -3.5;
    m0.at(1,1) = 37.0;

    fmt::print(
        "m0' =>\n{}\n",
        m0.format(alloc.adapt<char>())
          .payload_or("(NULL!)")
    );

    fmt::print(
        "m0' * m0' => \n{}\n",
        m0.multiply(alloc.adapt<field>(), m0)
          .format(alloc.adapt<char>())
          .payload_or("(NULL!)")
    );
}}

void markov_chain_test() noexcept {
    auto arena = core::arena_loud<core::stack_byte_allocator<std::byte,core::megabyte>>(core::megabyte, "logs/markov_chain_arena.log");
    using field = double;
    auto m0 = core::matrix<field, 101, 101>();
    m0.underlying.allocate(arena.adapt<field>(), m0.extent);

    for(core::size i = 0; i < m0.row_count; ++i) {
        for(core::size j = 0; j < m0.col_count; ++j) {
            if(i != j) continue;
            core::size idx0 = 0;
            if(j == 0) idx0 = m0.col_count - 1;
            else idx0 = j - 1;

            core::size idx1 = 0;
            if(j == m0.col_count - 1) idx1 = 0;
            else idx1 = j + 1;
            
            m0.at(i, idx0) = 0.5;
            m0.at(i, idx1) = 0.5;
        }
    }
    
    decltype(m0) buffer; 
    buffer.underlying.allocate(arena.adapt<field>(), buffer.extent);
    for(core::size k = 0; k < std::numeric_limits<core::size>::max(); ++k) {
        auto &&_ = m0.multiply(buffer.in_place_allocator(), m0);
        std::swap(buffer, m0);

        field col_sum = 0.0;
        for(core::size j = 0; j < m0.row_count; ++j) col_sum += m0.at(j, 0);
        constexpr static field tolerance = 0.0000001;
        if(std::abs(col_sum - field(1)) >= tolerance) {
            fmt::print("x == {}, leading to imprescision at k == {} with tolerance = {}\n", col_sum, k, tolerance);
            break;
        }
    }
    
    auto strg = m0.format(arena.adapt<char>());
    fmt::output_file("logs/markov_result.log")
    .print(
        "m0 big, str size is {}\n{}\n",
        strg.size(), strg.payload_or("nil")
    );
}

constexpr auto
memptr_testing2() noexcept -> void {
    auto arena = core::arena<std::allocator<std::byte>>(core::hectobyte);
    auto character_allocator = arena.adapt<char>();
    auto name = core::memptr<core::string<>>().allocate(arena.adapt<core::string<>>());
    core::construct_at(name).append(character_allocator, "[CA!]");
    fmt::print("name [gain] => {}\n", name->payload_or("(nil)"));
    core::destroy_at(name);
}

auto main() noexcept -> core::i32 {
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

    strings();

    fuzzing_strings();

    memptr_tests<
        core::memptr<core::string<>>,
        core::stack_byte_allocator<core::string<>,core::kilobyte>,
        core::stack_byte_allocator<char,core::kilobyte>
    >("tests/memptr-stack_alloc.out");
    core::run_test("memptr-stack_alloc");

    memptr_tests<
        core::memptr<core::string<>>,
        std::allocator<core::string<>>,
        std::allocator<char>
    >("tests/memptr-std::alloc.out");
    core::run_test("memptr-std::alloc");

    memptr_tests<
        core::memptr_unsafe<core::string<>>,
        std::allocator<core::string<>>,
        std::allocator<char>
    >("tests/memptr_unsafe-std::alloc.out");
    core::run_test("memptr_unsafe-std::alloc");

    memptr_tests<
        core::memptr_unsafe<core::string<>>,
        core::stack_byte_allocator<core::string<>,core::kilobyte>,
        core::stack_byte_allocator<char,core::kilobyte>
    >("tests/memptr_unsafe-stack_alloc.out");
    core::run_test("memptr_unsafe-stack_alloc");

    matrix_test();

    markov_chain_test();

    memptr_testing2();

    return EXIT_SUCCESS;
}

YOU CAN REBASE THIS
