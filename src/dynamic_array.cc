#include "dynamic_array.hpp"

int do_stuff() {
    darray<int> nums;
    nums.deb(1).deb(2).deb(3);
    int *begin = reinterpret_cast<int*>(nums.data);
    int sum = 0;
    for(int *num = begin; num < begin + nums.length; ++num) {
        sum += *num;
    }
    return sum;
}

void core::run_test(std::string_view testname) noexcept {
    std::string expect_name = "tests/";
    std::string out_name = expect_name;
    expect_name += testname;
    out_name += testname;
    expect_name += ".expect";
    out_name += ".out";

    auto expect = std::ifstream(expect_name);
    auto out = std::ifstream(out_name);

    assert(expect && "Bad Expect Name");
    assert(out && "Bad Out Name");

    std::string out_tester;
    std::string expect_tester;

    while((out >> out_tester) && (expect >> expect_tester)) {
        assert(out_tester == expect_tester);
    }
    assert(!((out >> out_tester) || (expect >> expect_tester)));
}

core::defer::defer(decltype(defered) defered_) noexcept
: defered(std::move(defered_)) {}

core::defer::~defer() noexcept {
    defered();
}
