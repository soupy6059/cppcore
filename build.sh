compiler="gcc"
debug="-g"
nodebug="-DNDEBUG"
std="-std=c++23"


warnings="-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Winvalid-constexpr \
-Wold-style-cast -Wcast-align -Wunused \
-Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion \
-Wnull-dereference -Wdouble-promotion -Wformat=2"

function do_and_speak {
    echo $@
    $@
}

do_and_speak clear
do_and_speak rm *.a
do_and_speak rm build/*
do_and_speak rm bin/*
do_and_speak rm tests/*.out
do_and_speak rm vgcore*
do_and_speak rm logs/*

include_fmt="-I ../fmt/include"
link_fmt="${include_fmt} -L ../fmt -lfmt -no-pie"

compile_source() {
    name=$(basename $1)
    do_and_speak $compiler -O3 $std $warnings $debug $include_fmt -c -Iinc/core -Iinc $src -o build/${name%.*}.o
}

for src in $(find src/*.cc)
do
    compile_source $src
done

for src in $(find src/core/*.cc)
do
    compile_source $src
done

libnames=()
for object in $(find build/*.o)
do
    name=$(basename $object)
    libnames+=(-l${name%.*})
    do_and_speak ar rcs lib${name%.*}.a $object
done

do_and_speak $compiler -O3 $std $warnings $debug test_it.cc $link_fmt -L. ${libnames[@]} -Iinc -o bin/test_it-quick
do_and_speak ./bin/test_it-quick
