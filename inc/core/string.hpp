#pragma once
#ifndef STRING__
#define STRING__

#include "core/core.hpp"
#include "core/memptr.hpp"

#include <cstring>

namespace core {
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
};
};
#endif
