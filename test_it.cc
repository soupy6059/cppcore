#include <string>
#include <functional>
#ifndef NDEBUG
#include <iostream>
#endif
#include <vector>
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

#include "core/allocator.hpp"
#include "core/pool.hpp"
#include "core/core.hpp"
#include "core/defer.hpp"
#include "core/matrix.hpp"
#include "core/memptr.hpp"
#include "core/string.hpp"


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
pool_test() {
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

    std::destroy_at(y);
    std::destroy_at(x);
    std::destroy_at(z);
}

[[nodiscard]] consteval bool
pool_ub_test() {
    auto alloc = core::alloc::typed<int,256>();
    decltype(auto) x = alloc.allocate(1);
    std::construct_at(x, 23);
    assert(*x == 23);
    std::destroy_at(x);
    return true;
}

void
pool_test2() {
    static constexpr std::size_t CAPACITY(1024);
    static auto storage = core::pool_loud<core::alloc::byte<std::byte,CAPACITY>>(CAPACITY, "tests/fibbo_cache_out.out");
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
        if(static_cast<core::size>(N) >= results_size) return -1;
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
}

template<typename T>
using no_op_t = decltype([](T){});

void pool_loud_test() {
    auto pool1 = core::pool_loud<core::alloc::byte<std::byte,core::kilobyte>>(core::kilobyte, "tests/pool_loud.out");
    assert(pool1.allocate<std::string>(1));
    
    std::unique_ptr<std::string,no_op_t<std::string*>> ptr;

    ptr.reset(pool1.allocate<std::string>(1));
    std::construct_at(ptr.get(), "Carter Aitken");
    assert(*ptr == "Carter Aitken");
    std::destroy_at(ptr.get());

    pool1.reset();
    ptr.reset(pool1.allocate<std::string>(1));
    new(ptr.get()) std::string("Carter Aitken2");
    ptr->operator+=("[ADDON!]");

    fmt::ostream out = fmt::output_file("tests/pool_loud.out");
    out.print("{}\n", *ptr);
    out.close();

    core::run_test("pool_loud");
    
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
    using core_string = std::basic_string<char,std::char_traits<char>, core::alloc::resetting<char,64>>;
    auto str0 = core_string("Carter Aitken");
    for(auto i [[maybe_unused]] : std::ranges::views::iota(0zu,10zu)) {
        if(str0.capacity() * 2 > 64) break;
        constexpr static std::string_view nums = "1234567890";
        str0 += nums;
    }

    auto alloc = core::pool_loud<core::alloc::byte<std::byte,1024>>(1024, "logs/string_test_allocator.log");
    decltype(auto) str1 = alloc.allocate<core_string>(1);
    std::construct_at(str1, "Carter Aitken");
    *str1 += "[SUPRISE!]";
    
    fmt::output_file("tests/custom_string.out").print("{:?}\n", *str1);
    core::run_test("custom_string");

    std::destroy_at(str1);

}

void
strings() {
    auto memory = core::alloc::byte<char,core::decabyte>();
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
    assert(name.payload.is_valid());
    fmt::print("{:?}, size = {}, capacity = {}\n", name.c_str(), name.length(), name.capacity());
    name.deallocate(memory);

    auto nofree_memory = core::pool_loud<core::alloc::byte<std::byte,core::kilobyte>>(core::kilobyte, "logs/nofree_memory.log");
    auto char_pool = nofree_memory.adapt<char>();
    core::string words;
    // Here note we haven't allocated our string yet, but since append
    // is passed an allocator, it's reasonable to assume it can allocate an
    // initial buffer.
    if(!words.append(char_pool, std::string_view("int main(void) { return EXIT_SUCCESS; }")).payload.is_valid()) {
        assert(false && "Bad Alloc!");
    }
    fmt::print("words == {:?}\n", words.c_str());

    std::ranges::for_each(
        std::ranges::views::iota(core::size(0), core::kilobyte),
        [&words, &char_pool](core::size i [[maybe_unused]]) {
            // words.append(char_pool, std::string_view("."));
            words.append(char_pool, '.');
        }
    );
    assert(words.payload.is_valid() && "shortting seems to be working!");
    fmt::print("word.size() == {}; word.capacity() == {}\n", words.length(), words.capacity());
    // should be about 64 bytes left in char_pool
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
    .print("{1}/{2} -> {0}\n", to_fuzz.c_str(), to_fuzz.length(), to_fuzz.capacity());
    to_fuzz.deallocate(alloc);
}

template<typename Ptr, typename StringAllocator, typename CharAllocator>
void memptr_tests(std::string_view filename) noexcept {
    auto str_storage = StringAllocator();
    auto char_storage = CharAllocator();
    auto name = Ptr();
    name.allocate(str_storage);
    if constexpr(std::is_same_v<Ptr,core::memptr_unsafe<core::string<>>>) {
        fmt::print("testing for bad alloc on str_storage\n");
        if(!name.is_valid()) {
            fmt::print("\tBAD ALLOC!YAYAYYAYYA\n");
            fmt::output_file(filename.data())
            .print("bad alloc on str_storage!\n");
            return;
        }
    }
    std::construct_at(name.get());
    name->allocate(char_storage, core::hexabyte);
    if constexpr(std::is_same_v<Ptr,core::memptr_unsafe<core::string<>>>) {
        if(!name->payload.is_valid()) {
            fmt::output_file(filename.data())
            .print("bad alloc on char_storage!\n");
            std::destroy_at(name.get());
            name.deallocate(str_storage);
            return;
        }
    }
    
    name->append(char_storage, std::string_view("Carter Aitken!"));

    fmt::output_file(filename.data()).print("memptr_tests() => {}\n", name->payload_or("(nil)"));
    
    name->deallocate(char_storage);
    std::destroy_at(name.payload);
    name.deallocate(str_storage);
}

void
matrix_test() {{
    auto alloc = std::allocator<double>();

    auto m0 = core::math::matrix<typename decltype(alloc)::value_type, 2, 2>();
    m0.underlying.allocate(alloc, m0.amount_to_allocate);
    std::uninitialized_value_construct_n(m0.underlying.payload, m0.amount_to_allocate);

    m0.at(0,0) = 2.0;
    m0.at(1,1) = 1.0;

    auto m1 = core::math::matrix<typename decltype(alloc)::value_type, 2, 3>();
    m1.underlying.allocate(alloc, m1.amount_to_allocate);
    std::uninitialized_value_construct_n(m1.underlying.payload, m1.amount_to_allocate);

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
    auto alloc = core::pool<core::alloc::byte<std::byte,core::kilobyte>>(core::kilobyte);
    using field = double;
    auto m0 = core::math::matrix<field, 2, 2>();
    m0.underlying.allocate(alloc.adapt<field>(), m0.amount_to_allocate);
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
    auto pool = core::pool_loud<core::alloc::byte<std::byte,core::megabyte>>(core::megabyte, "logs/markov_chain_pool.log");
    using field = double;
    auto m0 = core::math::matrix<field, 5, 5>();
    m0.underlying.allocate(pool.adapt<field>(), m0.amount_to_allocate);
    std::uninitialized_value_construct_n(m0.underlying.get(), m0.amount_to_allocate);

#if 0
    for(core::size i = 0; i < m0.row_count; ++i) {
        for(core::size j = 0; j < m0.col_count; ++j) {
            if(i != j) continue;
            
            m0.at(i, core::posisub(j, core::size(1), m0.col_count)) = 0.5;
            m0.at(i, core::posimod(j + 1, m0.col_count)) = 0.5;
        }
    }
#elif 0
    for(core::size i = 0; i < m0.row_count; ++i) {
        for(core::size j = 0; j < m0.col_count; ++j) {
            if(i != j) continue;
            
            m0.at(i, core::posimod(j + 1, m0.col_count)) = 1;
        }
    }
    m0.at(0, 0) = 0.5;
    m0.at(0, 1) = 1.0 - m0.at(0,0);
#else
    for(core::size i = 0; i < m0.row_count; ++i) {
        for(core::size j = 0; j < m0.col_count; ++j) {
            m0.at(i,j) = std::pow(0.5, static_cast<field>(j + 1));
        }
        m0.at(i,m0.col_count - 1) = m0.at(i,m0.col_count - 2);
    }
#endif

    fmt::print("initial markov input:\n{}\n", m0.format(pool.adapt<char>()).payload_or("(nil)"));
    
    auto dedicated_printing_alloc = core::pool<core::alloc::byte<std::byte,core::kilobyte * 10>>(core::kilobyte*10);

    decltype(m0) buffer1;
    decltype(m0) buffer2;

    // core::alloc::adapt<field>(pool)
    buffer1.underlying.allocate(pool.adapt<field>(), buffer1.amount_to_allocate);
    std::uninitialized_value_construct_n(buffer1.underlying.get(), buffer1.amount_to_allocate);
    for(core::size i = 0; i < buffer1.row_count; ++i) {
        buffer1.at(i,i) = field(1);
    }

    buffer2.underlying.allocate(pool.adapt<field>(), buffer2.amount_to_allocate);
    std::uninitialized_value_construct_n(buffer2.underlying.get(), buffer2.amount_to_allocate);

    auto file = fmt::output_file("logs/markov_computing.log");
    for(core::size k = 0; k < std::min(std::numeric_limits<core::size>::max(), core::size(300)); ++k) {
        auto &&g [[maybe_unused]] = buffer1.multiply(core::alloc::adapt(buffer2.underlying), m0);
        std::swap(buffer1, buffer2);

        {
            dedicated_printing_alloc.reset();
            core::string<> to_print = buffer1.format(dedicated_printing_alloc.adapt<core::string<>::char_t>());
            file.print("m0^({}) =>\n{}\n", k, to_print.payload_or("(nil)"));
        }

        field col_sum = 0.0;
        for(core::size j = 0; j < buffer1.col_count; ++j) col_sum += buffer1.at(0, j);
        constexpr static field tolerance = 0.0000001;
        if(std::abs(col_sum - field(1)) >= tolerance) {
            fmt::print("x == {}, leading to imprescision at k == {} with tolerance = {}\n", col_sum, k, tolerance);
            break;
        }
    }
    std::swap(buffer1, m0);
    
#if 0
    auto strg = m0.format(pool.adapt<char>());
    fmt::output_file("logs/markov_result.log")
    .print(
        "m0 big, str size is {}\n{}\n",
        strg.size(), strg.payload_or("nil")
    );
#elif 0
    fmt::output_file("logs/markov_result.log")
    .print("{}\n",
        m0.format(pool.adapt<core::string<>::char_t>())
        .payload_or("nil")
    );
#else
    fmt::output_file("logs/markov_result.log").print("markov result =>\n{}\n",
        m0.format(pool.adapt<core::string<>::char_t>())
        .payload_or("nil")
    );
#endif
}

auto
memptr_testing2() noexcept -> void {
    auto pool = core::pool<std::allocator<std::byte>>(core::hectobyte);
    auto character_allocator = pool.adapt<char>();
    auto name = core::memptr<core::string<>>().allocate(pool.adapt<core::string<>>());
    core::construct_at(name).append(character_allocator, "[CA!]");
    fmt::print("name [gain] => {}\n", name->payload_or("(nil)"));
    core::destroy_at(name);
}

auto
defer_testing() noexcept -> void {
    core::memptr<core::string<>> name;
    std::allocator<core::string<>> string_alloc;
    std::allocator<core::string<>::char_t> char_alloc;
    name.allocate(string_alloc);
    core::defer l0 ([=] mutable { name.deallocate(string_alloc); });
    
    core::construct_at(name)
    .append(char_alloc, "[CA]");
    auto l1 = core::defer([=] mutable {
        name->deallocate(char_alloc);
        core::destroy_at(name);
    });
    
    fmt::output_file("logs/defer_testing.log")
    .print("name => {}", name->payload_or("(nil)"));
}

auto
monad_testing() -> void {
    auto get_numbers = std::allocator<core::i32>();
    
    core::memptr<core::i32> x;
    core::memptr<core::i32> y;

    x.allocate(get_numbers);
    //core::defer _([=] mutable { x.deallocate(get_numbers); });
    core::construct_at(x, 32);

    auto square = [](core::i32 num) -> core::i32 {
        return num * num;
    };
    
    auto file = fmt::output_file("logs/monad_testing.log");
    auto print_num = [&file](core::i32 num) -> core::i32 {
        file.print("num == {}\n", num);
        return num;
    };

    auto remove_with = []<typename alloc_t>(alloc_t &&alloc) {
        return [=](core::i32 &num) mutable {
            if(num == core::i32(-1)) {
                std::forward<alloc_t>(alloc)
                .deallocate(std::addressof(num), 1);
                return core::memptr<core::i32>();
            }
            return core::memptr<core::i32>({std::addressof(num)}, {}, 1);
        };
    };

    auto &&l1[[maybe_unused]] = x.transform(square).transform(print_num)
                .and_then(remove_with(get_numbers))
                .transform(print_num)
                .transform([](core::i32) { return -1; })
                .and_then(remove_with(get_numbers))
                .transform(print_num);
    auto &&l2[[maybe_unused]] = y.transform(square).transform(print_num);
}

auto matrix_fun() noexcept -> void {
    auto pool = core::pool<
        core::alloc::byte<
            std::byte,
            core::kilobyte
        >
    >(core::kilobyte);
    
    using field = float;

    auto A = core::math::matrix<field, 5, 5>();
    A.underlying.allocate(pool.adapt<field>(), A.amount_to_allocate);
    std::uninitialized_value_construct_n(A.underlying.get(), A.underlying.extent);

    auto x = core::math::matrix<field, 5, 1>();
    x.underlying.allocate(pool.adapt<field>(), x.amount_to_allocate);
    std::uninitialized_value_construct_n(x.underlying.get(), x.underlying.extent);
   
    static_assert(std::forward_iterator<core::memptr<field>>); 
    static_assert(std::ranges::forward_range<core::memptr<field>>); 
    static_assert(std::ranges::view<core::memptr<field>>); 
    for(core::size n = 0; field &i: x.underlying
    | std::views::filter([&n](field) mutable { ++n; return n % 2 == 0; })) {
        i = static_cast<field>(n * n + 1);
    }

    for(core::size i = 0; i < A.row_count; ++i) {
        for(core::size j = 0; j < A.col_count; ++j) {
            A.at(i,j) = static_cast<field>(i * j + 1);
        }
    }

    auto y = A.multiply(pool.adapt<field>(), x);
    
    auto file = fmt::output_file("logs/matricies_and_ranges.log");
    file.print("x =>\n{}\n", x.format(pool.adapt<char>()).payload_or("(nil)"));
    file.print("A =>\n{}\n", A.format(pool.adapt<char>()).payload_or("(nil)"));
    file.print("A@x =>\n{}\n", y.format(pool.adapt<char>()).payload_or("(nil)"));
}


constexpr auto memptr_ranges = [] {
    auto pool = core::pool<
        core::alloc::byte<
            std::byte,
            core::kilobyte
        >
    >(core::kilobyte);

    core::memptr<std::vector<core::i32>> valid;
    core::memptr<std::vector<core::i32>> invalid;

    valid.allocate(pool.adapt<decltype(valid)::value_type>(), 1);
    core::construct_at(valid);

    auto pb = [](core::i32 num) {
        return [num](std::vector<core::i32> &v) {
            v.push_back(num);
        };
    };

    std::ranges::for_each(std::ranges::views::iota(core::i32(1), core::i32(6)),
    [=](core::i32 x) mutable {
        std::ranges::for_each(valid, pb(x));
        std::ranges::for_each(invalid, pb(x));
    });
    
    auto file = fmt::output_file("logs/range_testing.log");
    auto print_vector = [&file](std::vector<core::i32> &v) {
        std::ranges::for_each(v, [&file](core::i32 x) {
            file.print("{}\n", x);
        });
    };

    std::ranges::for_each(valid, print_vector);
    std::ranges::for_each(invalid, print_vector);
    
    core::destroy_at(valid);
};

auto testing_default_pool() {
    auto pool = core::pool<core::alloc::byte<std::byte,core::kilobyte>>(core::hexabyte * 2 + 1);
    auto l0 [[maybe_unused]] = pool.allocate<core::u8>(1);
    auto s0   = core::string<>();
    auto s1   = core::string<>();
    s0.allocate(pool.adapt<core::string<>::char_t>(), core::hexabyte);
    s1.allocate(pool.adapt<core::string<>::char_t>(), core::hexabyte);
    
    auto file = fmt::output_file("logs/strings_and_pools.log");
    auto log_string = [&file](std::string_view name, core::string<> s) {
        file.print("{} => {}.length() == {}; {}.capacity() == {}\n\t{:?}\n", name, name, s.length(), name, s.capacity(), s.c_str());
    };
#define LOG(X) do { log_string(#X, X); } while(false)

    for(decltype(pool)::difference_type i = 0; i < reinterpret_cast<std::byte*>(pool.capacity) - reinterpret_cast<std::byte*>(pool.payload); ++i) {
        s0.append(pool.adapt<core::string<>::char_t>(), '#');
        s1.append(pool.adapt<core::string<>::char_t>(), '@');
    }

    assert(s0.length() == std::strlen(s0.c_str()));
    assert(s1.length() == std::strlen(s1.c_str()));

    LOG(s0);
    LOG(s1);

#undef LOG
}

auto string_buffers() {
    auto pool = core::pool<core::alloc::byte<std::byte,core::kilobyte>>(core::kilobyte);
    auto chpool = pool.adapt<core::string<>::char_t>();
    auto s0 = core::string<>().allocate(chpool, core::hectobyte);
    auto buffer1 = core::string<>().allocate(chpool, core::hectobyte);
    auto buffer2 = core::string<>().allocate(chpool, core::hectobyte);

    assert(s0.payload.is_valid() && buffer1.payload.is_valid() && buffer2.payload.is_valid());
    assert(s0.capacity() == core::hectobyte && buffer1.capacity() == core::hectobyte && buffer2.capacity() == core::hectobyte);

    s0.append(core::alloc::adapt(s0.payload), '#');
    assert(s0.length() == 1);
    
    auto storage1 = core::alloc::adapt(buffer1.payload);
    auto storage2 = core::alloc::adapt(buffer2.payload);
    for(core::i32 i = 0; i < 20; ++i) {
        buffer2 = core::string<>::concat(storage2, buffer1, s0);
        std::swap(buffer1, buffer2);
        std::swap(storage1, storage2);

        assert(std::strlen(buffer1.data()) == buffer1.length());
        assert(std::strlen(buffer2.data()) == buffer2.length());
        assert(std::strlen(s0.data()) == s0.length());
    }
    fmt::print("buffer1 => {}\n", buffer1.c_str());
    fmt::print("s0 => {}\n", s0.c_str());

    core::memptr<core::i32> x;
    x.allocate(std::allocator<core::i32>());
    core::memptr<core::i32> y;
    y.allocate(core::alloc::adapt(x));
    *x = 32;
    assert(*x == *y);
    x.deallocate(std::allocator<core::i32>());
}

auto main() noexcept -> core::i32 {
    static_assert(pool_ub_test());
    pool_test2();
    pool_test2();
    pool_test2();
    pool_loud_test();
    leak_test();

    string_test();

    strings();

    fuzzing_strings();

    memptr_tests<
        core::memptr<core::string<>>,
        core::alloc::byte<core::string<>,core::kilobyte>,
        core::alloc::byte<char,core::kilobyte>
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
        core::alloc::byte<core::string<>,core::kilobyte>,
        core::alloc::byte<char,core::kilobyte>
    >("tests/memptr_unsafe-stack_alloc.out");
    core::run_test("memptr_unsafe-stack_alloc");

    memptr_tests<
        core::memptr<core::string<>>,
        core::alloc::byte<core::string<>,core::dualbyte>,
        core::alloc::byte<char,core::kilobyte>
    >("tests/memptr-no_memory_for_string_alloc.out");

    memptr_tests<
        core::memptr<core::string<>>,
        core::alloc::byte<core::string<>,core::dualbyte>,
        core::alloc::byte<char,core::dualbyte>
    >("tests/memptr-no_memory_for_char_alloc.out");

    memptr_tests<
        core::memptr_unsafe<core::string<>>,
        core::alloc::byte<core::string<>,core::kilobyte>,
        core::alloc::byte<char,core::dualbyte>
    >("tests/memptr_unsafe-no_memory_for_char_alloc.out");

    memptr_tests<
        core::memptr_unsafe<core::string<>>,
        core::alloc::byte<core::string<>,core::size(1)>,
        core::alloc::byte<char,core::hectobyte>
    >("tests/memptr_unsafe-no_memory_for_string_alloc.out");
    
    struct bad_string_allocator {
        core::string<> *allocate(core::size) {
            return nullptr;
        }
        void deallocate(core::string<>*,core::size) {}
    };
    memptr_tests<
        core::memptr_unsafe<core::string<>>,
        bad_string_allocator,
        core::alloc::byte<char,core::hectobyte>
    >("tests/memptr_unsafe-bad_allocator.out");

    matrix_test();

    markov_chain_test();

    memptr_testing2();

    defer_testing();

    monad_testing();

    pool_test();

    matrix_fun();

    memptr_ranges();

    testing_default_pool();

    string_buffers();

    return EXIT_SUCCESS;
}
