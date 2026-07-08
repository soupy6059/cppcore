#ifndef DEFER__
#define DEFER__
#pragma once

#include <functional>

namespace core {
struct defer {
    std::move_only_function<void()> defered;

    explicit defer(decltype(defered)) noexcept;

    ~defer() noexcept;
};
};

#endif
