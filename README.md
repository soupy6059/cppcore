# TODO
- Add a strict_access_memptr
- priptr "private_pointer"

```c++
template<typename T>
class priptr final {
    constexpr decltype(auto) allocate(auto &&alloc, size N) noexcept {
        payload = std::forward<decltype(alloc)>(alloc).allocate(N);
        extent = payload? N : size(0);
        return *this;
    }
private:
    T *payload = nullptr;
    size extent = 0;
};
```
