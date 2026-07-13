#ifndef MEMPTR__
#define MEMPTR__

#include <cstring>
#include <cassert>

namespace core {

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
    get() const noexcept -> T *{
        return payload;
    }

    [[nodiscard]] constexpr auto
    at(size i) const noexcept -> T &{
        return payload[i];
    }

    [[nodiscard]] constexpr auto
    operator*() const noexcept -> T& {
        return *payload;
    }

    [[nodiscard]] constexpr auto
    operator->() const noexcept -> T *{
        return payload;
    }

    [[nodiscard]] constexpr auto
    operator[](core::size i) const noexcept -> T &{
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
struct memptr: public memptr_unsafe<T>, public std::ranges::view_interface<memptr<T>> {
    size extent = 0;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using iterator_categroy = std::forward_iterator_tag;
    
    constexpr auto &operator++() noexcept {
        this->payload += 1;
        extent = extent == 0? 0 : extent - 1;
        return *this;
    }

    constexpr auto operator++(int) noexcept {
        auto saved = *this;
        ++*this;
        return saved;
    }

    constexpr auto operator==(memptr const other) const noexcept -> bool {
        return other.payload == this->payload;
    }

    constexpr auto begin() noexcept -> memptr {
        return *this;
    }

    constexpr auto end() noexcept {
        return memptr(memptr_unsafe(this->payload + extent), std::ranges::view_interface<memptr<T>>(), 0);
    }

    template<typename Alloc> constexpr decltype(auto)
    allocate(Alloc &&alloc, size N = 1) noexcept {
        this->payload = std::forward<Alloc>(alloc).allocate(N);
        extent = this->payload? N : size(0);
        return *this;
    }

    template<typename Alloc> constexpr decltype(auto)
    deallocate(Alloc &&alloc) noexcept {
        std::forward<Alloc>(alloc).deallocate(this->payload, extent);
        return *this;
    }

    template<typename Alloc> constexpr decltype(auto)
    reallocate(Alloc &&alloc, size N) noexcept {
#if 1
        memptr old = *this;
        allocate(alloc, N);
        if(!this->payload) {
            std::swap(old,*this);
            return *this;
        }

        size to_copy = std::min(old.extent, this->extent);
        std::memcpy(this->get(), old.get(), to_copy);
        old.deallocate(std::forward<Alloc>(alloc));
#else
        memptr new_data = allocate(alloc, N);
        std::memcpy(new_data.get(), this->get(), std::min(new_data.extent, this->extent));
        std::swap(new_data, *this);
        new_data.deallocate(std::forward<Alloc>(alloc));
#endif
        
        return *this;
    }

    [[nodiscard]] constexpr auto
    get_backup() const noexcept -> T &{
        static T backup{};
        return backup;
    }

    [[nodiscard]] constexpr auto
    value() const noexcept -> T &{
        if(extent == 0) return get_backup();
        return *this->payload;
    }

    [[nodiscard]] constexpr auto
    at(size i) const noexcept -> T &{
        if(extent <= i || extent == 0) return get_backup();
        return this->payload[i];
    }

    [[nodiscard]] constexpr auto
    operator*() const noexcept -> T& {
        return value();
    }

    [[nodiscard]] constexpr auto
    operator->() const noexcept -> T *{
        if(!this->payload) return std::addressof(get_backup());
        return this->payload;
    }

    [[nodiscard]] constexpr auto
    operator[](core::size i) const noexcept -> T &{
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
    get() const noexcept -> T *{
        if(!this->payload) return std::addressof(get_backup());
        return this->payload;
    }

    [[nodiscard]] constexpr auto
    is_valid() const noexcept -> bool {
        return this->payload;
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


};

#endif
