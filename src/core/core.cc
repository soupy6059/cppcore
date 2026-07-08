#include "core/core.hpp"

#include <fstream>
#include <string>
#include <cassert>
#include "fmt/core.h"

void core::run_test(std::string_view testname) noexcept {
    std::string expect_name = "tests/";
    std::string out_name = expect_name;
    expect_name += testname;
    out_name += testname;
    expect_name += ".expect";
    out_name += ".out";

    auto expect = std::ifstream(expect_name);
    auto out = std::ifstream(out_name);

    if(!expect || !out) {
        fmt::print("expect => [{}], out => [{}]\n", expect_name, out_name);
        assert(false && "BAD EXPECT OR BAD OUT");
    }

    std::string out_tester;
    std::string expect_tester;

    while((out >> out_tester) && (expect >> expect_tester)) {
        assert(out_tester == expect_tester);
    }
    assert(!((out >> out_tester) || (expect >> expect_tester)));
}
