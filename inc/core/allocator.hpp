#pragma once
#ifndef ALLOC__
#define ALLOC__

#include "core/core.hpp"
#include "fmt/core.h"
#include "fmt/os.h"

namespace core {
namespace alloc {

#ifdef SAFEPOINTER
template<typename T>
concept SafePointer = requires(T ptr) {
    ptr.get();
    ptr.extent;
};
#endif

template<typename T, std::size_t cap>
struct typed {
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;

    T payload[cap];
    size_type i = 0;
    
    template<typename U>
    struct rebind {
        using other = typed<U,cap>;
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
struct byte {
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    
    std::byte payload[cap];
    std::byte *current = payload;
    
    template<typename U>
    struct rebind {
        using other = byte<U,cap>;
    };

    constexpr auto allocate(size_type N) noexcept -> pointer {
        std::byte *saved = current;
        current += N * sizeof(T);
        if(current > payload + cap) {
            current -= N * sizeof(T);
            return nullptr;
        }
        return reinterpret_cast<pointer>(saved);
    }

    constexpr bool test(size_type N) noexcept {
        return current + N * sizeof(T) > payload + cap;
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
struct resetting {
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;

    alignas(T) std::byte payload[cap];

    template<typename U>
    struct rebind {
        using other = resetting<U,cap>;
    };

    constexpr pointer allocate(size_type N) {
        if(N * sizeof(T) > cap) { return nullptr; }
        return reinterpret_cast<T*>(payload);
    }
    constexpr void deallocate(pointer, size_type) noexcept {}
};

template<typename T>
struct adapt {
    using value_type = T;
    using size_type = size;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;

    T *payload;
    size extent;

    constexpr adapt(T *payload_, size extent_) noexcept
        : payload(payload_), extent(extent_) {}
#ifdef SAFEPOINTER
    template<SafePointer weird_pointer>
    constexpr adapt(weird_pointer &&ptr) noexcept
        : payload(ptr.get()), extent(std::forward<weird_pointer>(ptr).extent) {}
#endif

    template<typename U>
    struct rebind {
        using other = adapt<U>;
    };

    constexpr pointer allocate(size_type N) {
        if(N * sizeof(T) > extent) { return nullptr; }
        return reinterpret_cast<T*>(payload);
    }

    constexpr void deallocate(pointer, size_type) noexcept {}
};

#ifdef SAFEPOINTER
template<SafePointer weird_pointer>
adapt(weird_pointer &&ptr) -> adapt<typename weird_pointer::value_type>;
};
#endif

#endif
