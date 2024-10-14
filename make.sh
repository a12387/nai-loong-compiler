rm -r build
cmake -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build
./build/compiler -koopa hello.c -o hello.koopa