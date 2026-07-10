#pragma once
#ifndef POOL__
#define POOL__

#include "fmt/core.h"
#include <typeinfo>
#include <memory>
#include "fmt/os.h"

namespace core {

template<typename Alloc = std::allocator<std::byte>>
struct pool {
    using value_type = Alloc::value_type;
    using size_type = Alloc::size_type;
    using difference_type = Alloc::difference_type;

    void *payload;
    void *current;
    void *capacity;
    Alloc alloc{};
    
    constexpr pool(Alloc::size_type N) {
        payload = reinterpret_cast<void*>(alloc.allocate(N));
        current = payload;
        capacity = reinterpret_cast<std::byte*>(payload) + N;
    }

    constexpr pool &reset() {
        current = payload;
        return *this;
    }

    template<typename T> [[nodiscard]] 
    constexpr T *allocate(Alloc::size_type N) {
        auto size_left = reinterpret_cast<std::uintptr_t>(capacity) - reinterpret_cast<std::uintptr_t>(current);
        if(!std::align(alignof(T), sizeof(T) * N, current, size_left)) { return nullptr; }
        current = static_cast<std::byte*>(current) + sizeof(T) * N;
        return reinterpret_cast<T*>(current);
    }
    
    // Provides a thin allocator film over pools.  
    template<typename AdaptedT>
    struct adaptor {
        pool *parent;
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

    constexpr ~pool() noexcept {
        alloc.deallocate(
            reinterpret_cast<Alloc::value_type*>(payload), 
            static_cast<Alloc::size_type>(
                reinterpret_cast<std::uintptr_t>(capacity) - reinterpret_cast<std::uintptr_t>(payload)
            )
        );
    }
};

template<typename Alloc = std::allocator<std::byte>>
struct pool_loud: public pool<Alloc> {
    fmt::ostream out;
    pool_loud(Alloc::size_type N, auto &&filename): pool<Alloc>(N), out(fmt::output_file(filename)) {
        out.print("pool_loud({}, {:});\n", N, filename);
    }

    void print_stats() noexcept {
        out.print("\tused: {}; left: {};\n",
            reinterpret_cast<std::byte*>(this->current)
            - reinterpret_cast<std::byte*>(this->payload),
            reinterpret_cast<std::byte*>(this->capacity)
            - reinterpret_cast<std::byte*>(this->current)
        );
    }

    template<typename T = std::byte> [[nodiscard]] 
    constexpr T *allocate(Alloc::size_type N) {
        out.print("allocate<{}>({}); // allocates {} bytes\n", typeid(T).name(), N, N * sizeof(T));
        decltype(auto) saved = pool<Alloc>::template allocate<T>(N);
        print_stats();
        if(!saved) {
            out.print("\tBAD ALLOC! returning nullptr...\n");
        }
        return saved; 
    }

    constexpr pool_loud &reset() {
        out.print("reset();\n");
        print_stats();
        pool<Alloc>::reset();
        print_stats();
        return *this;
    }

    // Provides a thin allocator film over pools.  
    template<typename AdaptedT>
    struct adaptor_loud {
        pool_loud *parent;
        using value_type = Alloc::value_type;
        using size_type = Alloc::size_type;
        using difference_type = Alloc::difference_type;
        constexpr AdaptedT *allocate(size_type N) {
            assert(parent);
            parent->out.print("pool::adaptor<{}>({}) // allocated {} bytes\n", typeid(AdaptedT).name(), N, N * sizeof(AdaptedT));
            return parent->allocate<AdaptedT>(N);
        }
        constexpr adaptor_loud &deallocate(AdaptedT*,size_type) {
            return *this;
        }
    };

    template<typename AdaptT> [[nodiscard]]
    constexpr adaptor_loud<AdaptT> adapt() {
        out.print("pool adapting to allocator<{}>\n", typeid(AdaptT).name());
        return adaptor_loud<AdaptT>(this);
    }

    ~pool_loud() noexcept {
        out.print("~pool_loud()");
    }
};

};

#endif
