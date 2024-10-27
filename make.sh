rm -r build
rm hello.koopa
rm hello.o
cmake -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build
#./build/compiler -riscv hello.c -o hello.o
./build/compiler -koopa hello.c -o hello.koopa