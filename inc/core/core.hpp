#pragma once
#ifndef CORE__
#define CORE__

#include <cinttypes>
#include <string_view>

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


void run_test(std::string_view testname) noexcept;

};

#endif
