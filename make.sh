rm -r build
rm hello.koopa
cmake -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build
./build/compiler -koopa hello.c -o hello.koopa