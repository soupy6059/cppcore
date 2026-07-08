#ifndef MATRIX__
#define MATRIX__
#pragma once

#include "core/memptr.hpp"
#include "core/string.hpp"

namespace core {
namespace math {

template<typename T> constexpr auto
posimod(T x, T y) noexcept -> T {
    return ((x % y) + y) % y;
}

template<typename T> constexpr auto
posisub(T x, T y, T wrap) noexcept -> T {
    while(x < y) x += wrap;
    return x - y;
}

template<typename field, size row_count_, size col_count_>
struct matrix {
    memptr<field> underlying;
    static constexpr size row_count = row_count_;
    static constexpr size col_count = col_count_;
    static constexpr size amount_to_allocate = row_count * col_count;

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
        solution.underlying.allocate(std::forward<Alloc>(alloc), underlying.extent);
        if(!solution.underlying.payload) return solution;
        std::uninitialized_value_construct_n(solution.underlying.payload, underlying.extent);

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
        s.allocate(char_alloc, underlying.extent * 9 + row_count * 4);
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

};
};

#endif
