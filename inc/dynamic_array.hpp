#ifndef DYNAMIC_ARRAY
#define DYNAMIC_ARRAY
#pragma once

#include <memory>
#include <cstddef>
#include <cstring>
#include <cassert>
#include <type_traits>
#include <string>
#include <functional>

#ifndef NDEBUG
#include <iostream>
#endif


// arena_loud
#include "fmt/core.h"
#include "fmt/os.h"
#include <fstream>

template<typename T>
struct darray {
    std::size_t capacity{0};
    std::size_t length{0};
    void* data{nullptr};

    constexpr auto operator [] (this auto && self, std::size_t idx) noexcept -> T & {
        return * (static_cast <T*> (self.data) + idx);
    }

    constexpr auto operator [] (this auto const & self, std::size_t idx) noexcept -> T const & {
        return * (static_cast <T const*> (self.data) + idx);
    }

    constexpr darray (auto && ... elems) {
        static_assert ((std :: is_same_v <std :: remove_reference_t <decltype(elems)>, T> && ...));
        (this->dpb (std::forward <decltype(elems)> (elems)), ...);
    }

    constexpr darray (darray & other) {
        for (std :: size_t i = 0; i < other .length; ++i) {
            this->dpb (other [i]);
        }
    }

    constexpr darray (darray const &other) {
        for (std :: size_t i = 0; i < other .length; ++i) {
            this->dpb (other [i]);
        }
    }

    constexpr darray (darray && other) {
        std::swap(this->length, other.length);
        std::swap(this-> capacity, other.capacity);
        std::swap(this->data, other.data);
    }

    constexpr decltype(auto)
    pb(this auto&& self, auto&& item) noexcept {
        if(!self.resize_guard()) return self;
        self.do_the_push_back(std::forward<decltype(item)>(item));
        return self;
    }

//     constexpr decltype(auto)
//     pb(this auto &&self, auto const &item) noexcept {
//         #ifndef NDEBUG
//             std::cout << __PRETTY_FUNCTION__ << '\n';
//         #endif
//         if(!self.resize_guard()) return self;
//         self.do_the_push_back(item);
//         return self;
//     }

    constexpr decltype(auto)
    dpb(this auto&& self, auto&& item) noexcept {
        if(!self.resize_guard_destructive()) return self;
        self.do_the_push_back(std::forward<decltype(item)>(item));
        return self;
    }

    constexpr decltype(auto)
    eb(this auto&& self, auto&&...args) noexcept {
        if(!self.resize_guard()) return self;
        self.do_the_emplace_back(std::forward<decltype(args)>(args)...);
        return self;
    }

    constexpr decltype(auto)
    deb(this auto&& self, auto&&...args) noexcept {
        if(!self.resize_guard_destructive()) return self;
        self.do_the_emplace_back(std::forward<decltype(args)>(args)...);
        return self;
    }

    ~darray() {
        auto first = static_cast<T*>(data);
        std::destroy(first, first + length);
        std::free(data);
    }

private:
    constexpr decltype(auto)
    do_the_emplace_back(this auto&& self, auto&&...args) noexcept {
        std::construct_at(static_cast<T*>(self.data) + self.length++, std::forward<decltype(args)>(args)...);
    }

    constexpr decltype(auto)
    do_the_push_back(this auto&& self, auto&& item) noexcept {
        if constexpr(std::is_rvalue_reference_v<decltype(item)>) {
            std::uninitialized_move_n(&item, 1zu, static_cast<T*>(self.data) + self.length++);
        } else {
            std::uninitialized_copy_n(&item, 1zu, static_cast<T*>(self.data) + self.length++);
        }
    }

//     constexpr decltype(auto)
//     do_the_push_back(this auto &&self, auto const &item) {
//         std::uninitialized_copy_n(&item, 1zu, static_cast<T*>(self.data) + self.length++);
//     }

    int resize_guard(this auto&& self) noexcept {
        if(self.capacity != self.length) return 1;
        auto capacity_new = std::max(1zu, self.capacity * 2);
        void* data_new = std::aligned_alloc(alignof(T), sizeof(T) * capacity_new);
        if(!data_new) {
            #ifndef NDEBUG
                std::cerr << "failure to allocate memory\n";
            #endif
            return 0;
        }

        auto first_new = static_cast<T*>(data_new);

        auto first = static_cast<T*>(self.data);
        auto last = first + self.length;

        std::uninitialized_move_n(first, self.length, first_new);

        std::destroy(first, last);
        if(self.data) {
            std::free(self.data);
        }

        self.data = data_new;
        self.capacity = capacity_new;
        return 1;
    }

    int resize_guard_destructive(this auto&& self) noexcept {
        if(self.capacity != self.length) return 1;
        auto capacity_new = std::max(1zu, self.capacity * 2);
        void* data_new = std::aligned_alloc(alignof(T), sizeof(T) * capacity_new);
        if(!data_new) {
            #ifndef NDEBUG
                std::cerr << "failure to allocate memory\n";
            #endif
            return 0;
        }

        std::memcpy(data_new, self.data, sizeof(T) * self.length);

        if(self.data) { std::free(self.data); }

        self.data = data_new;
        self.capacity = capacity_new;
        return 1;
    }
};

int do_stuff();

namespace core {
    using size = std::size_t;
    constexpr static auto megabyte = size(1024zu*1024zu);
    constexpr static auto kilobyte = size(1024zu);
    constexpr static auto prekilobyte = size(512zu);
    constexpr static auto hectobyte = size(256zu);
    constexpr static auto prehepta = size(128zu);
    constexpr static auto decabyte = size(64zu);
    constexpr static auto predecabyte = size(32zu);
    constexpr static auto hexabyte = size(16zu);
    constexpr static auto octobyte = size(8zu);
    constexpr static auto quadbyte = size(4zu);
    constexpr static auto dualbyte = size(2zu);

    using i64 = std::int64_t;
    using i32 = std::int32_t;
    using i16 = std::int16_t;
    using i8 = std::int8_t;
    
    using u64 = std::uint64_t;
    using u32 = std::uint32_t;
    using u16 = std::uint16_t;
    using u8 = std::uint8_t;
    
    template<typename Alloc = std::allocator<std::byte>>
    struct arena {
        using value_type = Alloc::value_type;
        using size_type = Alloc::size_type;
        using difference_type = Alloc::difference_type;

        value_type *payload;
        value_type *current;
        value_type *capacity;
        Alloc alloc{};
        
        constexpr arena(Alloc::size_type N) {
            payload = alloc.allocate(N);
            current = payload;
            capacity = payload + N;
        }

        constexpr arena &reset() {
            current = payload;
            return *this;
        }

        template<typename T> [[nodiscard]] 
        constexpr T *allocate(Alloc::size_type N) {
            decltype(auto) saved = current;
            current += N * sizeof(T);
            if(current > capacity) {
                current -= N * sizeof(T);
                return nullptr;
            }
            return reinterpret_cast<T*>(saved);
        }
        
        // Provides a thin allocator film over arenas.  
        template<typename AdaptedT>
        struct adaptor {
            arena *parent;
            using value_type = Alloc::value_type;
            using size_type = Alloc::size_type;
            using difference_type = Alloc::difference_type;
            constexpr AdaptedT *allocate(size_type N) {
                return parent->allocate<AdaptedT>(N);
            }
            constexpr adaptor &deallocate(AdaptedT*,size_type) {
                return *this;
            }
        };

        template<typename AdaptT> [[nodiscard]]
        constexpr adaptor<AdaptT> adapt() {
            return adaptor<AdaptT>(this);
        }

        constexpr ~arena() noexcept {
            alloc.deallocate(payload, static_cast<Alloc::size_type>(capacity - payload));
        }
    };

    template<typename Alloc = std::allocator<std::byte>>
    struct arena_loud: public arena<Alloc> {
        fmt::ostream out;
        arena_loud(Alloc::size_type N, auto &&filename): arena<Alloc>(N), out(fmt::output_file(filename)) {
            out.print("arena_loud({}, {:});\n", N, filename);
        }

        void print_stats() noexcept {
            out.print("\tused: {}; left: {};\n",
                static_cast<Alloc::size_type>(this->current - this->payload),
                static_cast<Alloc::size_type>(this->capacity - this->current)
            );
        }

        template<typename T = std::byte> [[nodiscard]] 
        constexpr T *allocate(Alloc::size_type N) {
            out.print("allocate<{}>({}); // allocates {} bytes\n", typeid(T).name(), N, N * sizeof(T));
            decltype(auto) saved = arena<Alloc>::template allocate<T>(N);
            print_stats();
            if(!saved) {
                out.print("\tBAD ALLOC! returning nullptr...\n");
            }
            return saved; 
        }

        constexpr arena_loud &reset() {
            out.print("reset();\n");
            print_stats();
            arena<Alloc>::reset();
            print_stats();
            return *this;
        }

        // Provides a thin allocator film over arenas.  
        template<typename AdaptedT>
        struct adaptor_loud {
            arena_loud *parent;
            using value_type = Alloc::value_type;
            using size_type = Alloc::size_type;
            using difference_type = Alloc::difference_type;
            constexpr AdaptedT *allocate(size_type N) {
                parent->out.print("arena::adaptor<{}>({}) // allocated {} bytes\n", typeid(AdaptedT).name(), N, N * sizeof(AdaptedT));
                return parent->allocate<AdaptedT>(N);
            }
            constexpr adaptor_loud &deallocate(AdaptedT*,size_type) {
                return *this;
            }
        };

        template<typename AdaptT> [[nodiscard]]
        constexpr adaptor_loud<AdaptT> adapt() {
            out.print("arena adapting to allocator<{}>\n", typeid(AdaptT).name());
            return adaptor_loud<AdaptT>(this);
        }

        ~arena_loud() noexcept {
            out.print("~arena_loud()");
        }
    };

    template<typename T, std::size_t cap>
    struct stack_typed_allocator {
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;

        T payload[cap];
        size_type i = 0;
        
        template<typename U>
        struct rebind {
            using other = stack_typed_allocator<U,cap>;
        };
        
        constexpr pointer allocate(size_type N) {
            decltype(auto) location = payload + i;
            if(location + N > payload + cap) {
                return nullptr;
            }
            i += N;
            return location;
        }

        constexpr void deallocate(pointer, size_type) noexcept {}

        constexpr void construct(pointer ptr, auto &&...args) noexcept {
            std::construct_at(ptr, std::forward<decltype(args)>(args)...);
        }

        constexpr void destroy(pointer ptr) noexcept {
            std::destroy_at(ptr);
        }
    };

    template<typename T, std::size_t cap>
    struct stack_byte_allocator {
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;

        alignas(T) std::byte payload[cap * sizeof(T)];
        size_type i = 0;
        
        template<typename U>
        struct rebind {
            using other = stack_byte_allocator<U,cap>;
        };

        constexpr pointer allocate(size_type N) {
            decltype(auto) location = payload + i * sizeof(T);
            if(location + N * sizeof(T) > payload + cap * sizeof(T)) {
                return nullptr;
            }
            i += N;
            return reinterpret_cast<T*>(location);
        }

        constexpr bool test(size_type N) {
            decltype(auto) location = payload + i * sizeof(T);
            if(location + N * sizeof(T) > payload + cap * sizeof(T)) {
                return false;
            }
            return true;
        }

        constexpr void deallocate(pointer, size_type) noexcept {}

        constexpr void construct(pointer ptr, auto &&...args) noexcept {
            std::construct_at(ptr, std::forward<decltype(args)>(args)...);
        }

        constexpr void destroy(pointer ptr) noexcept {
            std::destroy_at(ptr);
        }
    };

    template<typename T, std::size_t cap>
    struct stack_byte_resetting_allocator {
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;

        alignas(T) std::byte payload[cap * sizeof(T)];

        template<typename U>
        struct rebind {
            using other = stack_byte_resetting_allocator<U,cap>;
        };

        constexpr pointer allocate(size_type N) {
            if(N > cap) { return nullptr; }
            return reinterpret_cast<T*>(payload);
        }
        constexpr void deallocate(pointer, size_type) noexcept {}
    };

    
    template<size N>
    using capstr = std::basic_string<char,std::char_traits<char>, core::stack_byte_resetting_allocator<char,N>>;

    void run_test(std::string_view testname) noexcept;
    template<typename T>
    requires requires { T{}; }
    struct memptr_unsafe {
        T *payload = nullptr;

        template<typename Alloc> constexpr decltype(auto)
        allocate(Alloc &&alloc, size N = 1) noexcept {
            payload = std::forward<Alloc>(alloc).allocate(N);
            return *this;
        }

        template<typename Alloc> constexpr decltype(auto)
        deallocate(Alloc &&alloc, size N = 1) noexcept {
            std::forward<Alloc>(alloc).deallocate(payload, N);
            return *this;
        }

        [[nodiscard]] constexpr auto
        value() noexcept -> T &{
            return *payload;
        }

        [[nodiscard]] constexpr auto
        get() noexcept -> T *{
            return payload;
        }

        [[nodiscard]] constexpr auto
        get() const noexcept -> T const *const{
            return payload;
        }

        [[nodiscard]] constexpr auto
        at(size i) noexcept -> T &{
            return payload[i];
        }

        [[nodiscard]] constexpr auto
        at(size i) const noexcept -> const T &{
            return payload[i];
        }

        [[nodiscard]] constexpr auto
        operator*() noexcept -> T& {
            return *payload;
        }

        [[nodiscard]] constexpr auto
        operator->() noexcept -> T *{
            return payload;
        }

        [[nodiscard]] constexpr auto
        operator[](core::size i) noexcept -> T &{
            return payload[i];
        }

        template<typename F> [[nodiscard]] constexpr auto
        and_then(F &&func) & -> memptr_unsafe {
            if(!payload) return *this;
            return std::forward<F>(func)(*payload);
        }

        template<typename F> [[nodiscard]] constexpr auto
        and_then(F &&func) && -> memptr_unsafe {
            if(!payload) return *this;
            return std::forward<F>(func)(std::move(*payload));
        }

        template<typename F> [[nodiscard]] constexpr auto
        transform(F &&func) & -> memptr_unsafe {
            if(!payload) return *this;
            *payload = std::forward<F>(func)(*payload);
            return *this;
        }

        template<typename F> [[nodiscard]] constexpr auto
        transform(F &&func) && -> memptr_unsafe {
            if(!payload) return *this;
            *payload = std::forward<F>(func)(std::move(*payload));
            return *this;
        }

        [[nodiscard]] constexpr auto
        is_valid() noexcept -> bool {
            return payload;
        }
    };

    template<typename T>
    struct memptr: public memptr_unsafe<T> {
        size extent = 0;
        template<typename Alloc> constexpr decltype(auto)
        allocate(Alloc &&alloc, size N = 1) noexcept {
            extent = N;
            this->payload = std::forward<Alloc>(alloc).allocate(N);
            if(!this->payload) extent = size(0);
            return *this;
        }

        template<typename Alloc> constexpr decltype(auto)
        deallocate(Alloc &&alloc) noexcept {
            std::forward<Alloc>(alloc).deallocate(this->payload, extent);
            return *this;
        }

        [[nodiscard]] constexpr auto
        get_backup() noexcept -> T &{
            static T backup{};
            return backup;
        }

        [[nodiscard]] constexpr auto
        get_backup() const noexcept -> T const &{
            static T backup{};
            return backup;
        }

        [[nodiscard]] constexpr auto
        value() noexcept -> T &{
            if(extent == 0) return get_backup();
            return *this->payload;
        }

        [[nodiscard]] constexpr auto
        at(size i) noexcept -> T &{
            if(extent <= i || extent == 0) return get_backup();
            return this->payload[i];
        }

        [[nodiscard]] constexpr auto
        at(size i) const noexcept -> T const &{
            if(extent <= i || extent == 0) return get_backup();
            return this->payload[i];
        }

        [[nodiscard]] constexpr auto
        operator*() noexcept -> T& {
            return value();
        }

        [[nodiscard]] constexpr auto
        operator*() const noexcept -> T const & {
            return value();
        }

        [[nodiscard]] constexpr auto
        operator->() noexcept -> T *{
            if(!this->payload) return std::addressof(get_backup());
            return this->payload;
        }

        [[nodiscard]] constexpr auto
        operator->() const noexcept -> T const *const {
            if(!this->payload) return std::addressof(get_backup());
            return this->payload;
        }

        [[nodiscard]] constexpr auto
        operator[](core::size i) noexcept -> T &{
            return at(i);
        }

        [[nodiscard]] constexpr auto
        operator[](core::size i) const noexcept -> T const &{
            return at(i);
        }

        template<typename F> [[nodiscard]] constexpr auto
        and_then(F &&func) -> memptr {
            if(!this->payload) return *this;
            return std::forward<F>(func)(*this->payload);
        }

        template<typename F> [[nodiscard]] constexpr auto
        transform(F &&func) -> memptr {
            if(!this->payload) return *this;
            *this->payload = std::forward<F>(func)(*this->payload);
            return *this;
        }

        [[nodiscard]] constexpr auto
        get() noexcept -> T *{
            if(!this->payload) return std::addressof(get_backup());
            return this->payload;
        }

        [[nodiscard]] constexpr auto
        get() const noexcept -> T const *const{
            if(!this->payload) return std::addressof(get_backup());
            return this->payload;
        }

        [[nodiscard]] constexpr auto
        is_valid() const noexcept -> bool {
            return this->payload;
        }
    };

    template<typename char_t_ = char>
    struct string {
        using char_t = char_t_;
        memptr<char_t> payload;
        core::size len = 0;

        template<typename Alloc>
        constexpr decltype(auto) allocate(Alloc &&alloc, core::size N = core::decabyte) noexcept
        requires requires {
            alloc.allocate(N);
        } {
            payload.allocate(std::forward<Alloc>(alloc), N);
            std::memset(payload.get(), '\0', payload.extent);
            return *this;
        }

        template<typename Alloc>
        constexpr decltype(auto) deallocate(Alloc &&alloc) noexcept
        requires requires {
            alloc.allocate(size(0));
        } {
            payload.deallocate(std::forward<Alloc>(alloc));
            len = 0;
            return *this;
        }

        [[nodiscard]] constexpr decltype(auto) at(core::size i) {
            return payload[i];
        }

        struct boring_char {
            char_t payload;
            size size_ = 1;
            [[nodiscard]] constexpr auto size() noexcept {
                return size_;
            }
            [[nodiscard]] constexpr char_t *data() noexcept {
                return &payload;
            }
        };

        template<typename Alloc, typename StrViewLike>
        constexpr decltype(auto) append(Alloc &&alloc, StrViewLike that) noexcept
        requires requires {
            that.size();
        } {
            if(!payload.is_valid()) { allocate(alloc); }
            if(!payload.is_valid()) { return *this; }

            core::size N = that.size();
            core::size cap_new = payload.extent;
            while(cap_new - len < N + core::size(1)) { cap_new *= 2; }
            if(cap_new != payload.extent) {
                decltype(payload) payload_new;
                payload_new.allocate(alloc, cap_new);
                if(!payload_new.is_valid()) { return *this; }
                std::memset(payload_new.get(), '\0', payload_new.extent);
                std::memcpy(payload_new.get(), payload.get(), payload.extent);
                payload.deallocate(std::forward<Alloc>(alloc));

                payload = payload_new;
            }

            std::memcpy(payload.get() + len, that.data(), N);
            len += N;

            return *this;
        }

        template<typename Alloc, typename StrViewLike>
        constexpr decltype(auto) append(Alloc &&alloc, StrViewLike that) {
            return append(std::forward<Alloc>(alloc), std::string_view(that));
        }

        template<typename Alloc>
        constexpr decltype(auto) append(Alloc alloc, char_t ch) noexcept {
            return append(std::forward<Alloc>(alloc), boring_char{
                .payload = ch,
                .size_ = core::size(1),
            });
        }

        [[nodiscard]] constexpr const core::size &size() {
            return len;
        }

        [[nodiscard]] constexpr const core::size &capacity() {
            return payload.extent;
        }

        [[nodiscard]] constexpr const char_t *const c_str() {
            return payload.get();
        }

        [[nodiscard]] constexpr auto
        payload_or(const char_t *const backup)
        noexcept -> const char_t *const{
            if(!payload.is_valid()) return backup;
            return payload.get();
        }

        constexpr operator bool() noexcept {
            return payload.get();
        }
    };


    template<typename T, typename ...Args> constexpr auto
    construct_at(memptr<T> ptr, Args &&...args) noexcept -> T &{
        if(!ptr.payload) return ptr.get_backup();
        std::construct_at(ptr.payload, std::forward<Args>(args)...);
        return *ptr;
    }

    template<typename T, typename ...Args> constexpr auto
    construct_at(memptr_unsafe<T> ptr, Args &&...args) noexcept -> T &{
        std::construct_at(ptr.payload, std::forward<Args>(args)...);
        return *ptr;
    }

    template<typename T> constexpr auto
    destroy_at(memptr<T> ptr) noexcept -> void {
        if(!ptr.payload) return;
        std::destroy_at(ptr.payload);
    }

    template<typename T> constexpr auto
    destroy_at(memptr_unsafe<T> ptr) noexcept -> void {
        std::destroy_at(ptr.payload);
    }

    template<typename field, size row_count_, size col_count_>
    struct matrix {
        memptr<field> underlying;
        static constexpr size row_count = row_count_;
        static constexpr size col_count = col_count_;
        static constexpr size extent = row_count * col_count;

        [[nodiscard]] constexpr field &at(size i, size j) noexcept {
            return underlying.at(i + j * row_count);
        }

        [[nodiscard]] constexpr field const &at(size i, size j) const noexcept {
            return underlying.at(i + j * row_count);
        }

        template<typename Alloc, size other_row_count, size other_col_count>
        [[nodiscard]] constexpr auto 
        multiply(Alloc &&alloc, matrix<field, other_row_count, other_col_count> const &other) noexcept
        -> matrix<field, row_count, other_col_count> {
            matrix<field, row_count, other_col_count> solution;
            solution.underlying.allocate(std::forward<Alloc>(alloc), extent);
            if(!solution.underlying.payload) return solution;
            std::uninitialized_value_construct_n(solution.underlying.payload, extent);

            for(size sol_row = 0; sol_row < solution.row_count; ++sol_row) {
                for(size sol_col = 0; sol_col < solution.col_count; ++sol_col) {
                    for(size k = 0; k < this->col_count; ++k) {
                        solution.at(sol_row,sol_col) += this->at(sol_row, k) * other.at(k, sol_col);
                    }
                }
            }

            return solution;
        }

        struct in_place_allocator_t {
            using value_type = field;
            using size_type = size;
            using difference_type = std::ptrdiff_t;
            using pointer = field*;

            matrix<field,row_count,col_count> src;

            [[nodiscard]] constexpr auto
            allocate(size_type)
            noexcept -> pointer {
                return src.underlying.payload;
            }

            [[nodiscard]] constexpr auto
            deallocate(pointer,size_type) noexcept {}
        };

        [[nodiscard]] constexpr auto
        in_place_allocator() noexcept -> in_place_allocator_t {
            return { *this };
        }        
        
        template<typename Alloc>
        [[nodiscard]] constexpr auto
        format(Alloc &&char_alloc) noexcept {
            auto s = string<>(); 
            s.allocate(char_alloc, extent * 9 + row_count * 4);
            for(size i = 0; i < row_count; ++i) {
                s.append(char_alloc, '[');
                for(size j = 0; j < col_count; ++j) {
                    s.append(char_alloc, std::to_string(at(i, j)));
                    s.append(char_alloc, ' ');
                }
                s.append(char_alloc, ']');
                if(row_count != 0 && i < row_count - 1) s.append(char_alloc, '\n');
            }
            return s;
        }
    };

    template<typename T> constexpr auto
    posimod(T x, T y) noexcept -> T {
        return ((x % y) + y) % y;
    }

    template<typename T> constexpr auto
    posisub(T x, T y, T wrap) noexcept -> T {
        while(x < y) x += wrap;
        return x - y;
    }

    struct defer {
        std::move_only_function<void()> defered;

        explicit defer(decltype(defered)) noexcept;

        ~defer() noexcept;
    };
}; // core

#endif
