#include "core/defer.hpp"

core::defer::defer(decltype(defer::defered) defered_) noexcept
: defered(std::move(defered_)) {}

core::defer::~defer() noexcept {
    defered();
}
