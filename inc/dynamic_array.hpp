#ifndef DYNAMIC_ARRAY
#define DYNAMIC_ARRAY
#pragma once

#include <memory>
#include <cstddef>
#include <cstring>

#ifndef NDEBUG
#include <iostream>
#endif
#include <cassert>
#include <string>

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
                return parent->allocate<AdaptedT>(N);
            }
            constexpr adaptor_loud &deallocate(AdaptedT*,size_type) {
                return *this;
            }
        };

        template<typename AdaptT> [[nodiscard]]
        constexpr adaptor_loud<AdaptT> adapt() {
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

    template<typename char_t = char>
    struct string {
        char_t *payload = nullptr;
        core::size len = 0;
        core::size cap = 0;

        template<typename Alloc>
        constexpr decltype(auto) allocate(Alloc &&alloc, core::size N = core::decabyte) {
            payload = std::forward<Alloc>(alloc).allocate(N);
            if(!payload) return *this;
            cap = N;
            std::memset(payload, '\0', N);
            return *this;
        }

        template<typename Alloc>
        constexpr decltype(auto) deallocate(Alloc &&alloc) {
            std::forward<Alloc>(alloc).deallocate(payload, cap);
            payload = nullptr;
            len = 0;
            cap = 0;
            return *this;
        }

        [[nodiscard]] constexpr decltype(auto) at(core::size i) {
            return payload[i];
        }

        template<typename Alloc, typename StrViewLike>
        constexpr decltype(auto) append(Alloc &&alloc, StrViewLike that) {
            if(!*this && !size() && !capacity()) {
                allocate(alloc);
                if(!*this) {
                    fmt::print("bad alloc on initial append!");
                    return *this;
                }
            }
            core::size N = that.size();
            core::size cap_new = cap;
            while(cap_new - len < N + core::size(1)) { cap_new *= 2; }
            if(cap_new != cap) {
                char_t *payload_new = alloc.allocate(cap_new);
                if(!payload_new) return *this;
                std::memset(payload_new, '\0', cap_new);
                std::memcpy(payload_new, payload, cap);
                std::forward<Alloc>(alloc).deallocate(payload, cap);

                cap = cap_new;
                payload = payload_new;
            }

            std::memcpy(payload + len, that.data(), N);
            len += N;

            return *this;
        }

        [[nodiscard]] constexpr const core::size &size() {
            return len;
        }

        [[nodiscard]] constexpr const core::size &capacity() {
            return cap;
        }

        [[nodiscard]] constexpr const char_t *const c_str() {
            return payload;
        }

        constexpr operator bool() noexcept {
            return payload;
        }
    };
}; // core

#endif
