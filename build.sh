compiler="clang++"
debug="-g"
nodebug="-DNDEBUG"
std="-std=c++23"

warnings="-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Winvalid-constexpr \
-Wno-init-list-lifetime -Wold-style-cast -Wcast-align -Wunused \
-Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion \
-Wnull-dereference -Wdouble-promotion -Wformat=2 -Wduplicated-cond \
-Wduplicated-branches -Wlogical-op -Wuseless-cast"

# CLANG++
warnings="-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Winvalid-constexpr \
-Wold-style-cast -Wcast-align -Wunused \
-Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion \
-Wnull-dereference -Wdouble-promotion -Wformat=2"


function do_and_speak {
    echo $@
    $@
}

do_and_speak rm *.a
do_and_speak rm build/*
do_and_speak rm bin/*
do_and_speak rm tests/*.out
do_and_speak rm vgcore*

include_fmt="-I ../fmt/include"
link_fmt="${include_fmt} -L ../fmt -lfmt -no-pie"

# build release object
do_and_speak $compiler -O3 $std $warnings $debug -c -Iinc src/dynamic_array.cc $include_fmt -o build/dynamic_array-quick-debug.o
# archive object
do_and_speak ar rcs libdynamic_array-quick-debug.a build/dynamic_array-quick-debug.o
# build test
do_and_speak $compiler -O3 $std $warnings $debug test_it.cc $link_fmt -L. -ldynamic_array-quick-debug -Iinc -o bin/test_it-quick-debug

echo ================== RUNNING test_it-quick-test ==================================
do_and_speak ./bin/test_it-quick-debug
echo ================== ENDING test_it-quick-test ===================================

# build release object
do_and_speak $compiler -O3 $std $warnings $nodebug -c -Iinc src/dynamic_array.cc $include_fmt -o build/dynamic_array-release.o
# archive object
do_and_speak ar rcs libdynamic_array-release.a build/dynamic_array-release.o
# build test
do_and_speak $compiler -O3 $std $warnings $nodebug test_it.cc $link_fmt -L. -ldynamic_array-release -Iinc -o bin/test_it-release

echo ================== RUNNING test_it-release ============================================
do_and_speak ./bin/test_it-release
echo ================== ENDING test_it-release =============================================


for opt in -O0 -O1 -O2 -O3
do
    do_and_speak $compiler $opt $std $warnings $debug -c -Iinc src/dynamic_array.cc $include_fmt -o build/dynamic_array-${opt}.o
    do_and_speak $compiler $opt -fsanitize=address $std $warnings $debug -c -Iinc src/dynamic_array.cc $include_fmt -o build/dynamic_array-${opt}-SA.o
done

for opt in -O0 -O1 -O2 -O3
do
    do_and_speak ar rcs libdynamic_array-${opt}.a build/dynamic_array-${opt}.o
    do_and_speak ar rcs libdynamic_array-${opt}-SA.a build/dynamic_array-${opt}-SA.o
done


for opt in -O0 -O1 -O2 -O3
do
    do_and_speak $compiler -O3 $std $warnings $debug test_it.cc $link_fmt -L. -ldynamic_array-${opt} -Iinc -o bin/test_it-${opt}
    do_and_speak $compiler -fsanitize=address -O3 $std $warnings $debug test_it.cc $link_fmt -L. -ldynamic_array-${opt}-SA -Iinc -o bin/test_it-${opt}-SA
done

for opt in -O0 -O1 -O2 -O3
do
    echo ================== RUNNING valgrind test_it-${opt} ===================
    do_and_speak valgrind ./bin/test_it-${opt}
    echo ================== ENDING valgrind test_it-${opt} ====================
    echo ================== RUNNING test_it-${opt}-address-sanitizer ==========
    do_and_speak ./bin/test_it-${opt}-SA
    echo ================== ENDING test_it-${opt}-address-sanitizer ===========
done

