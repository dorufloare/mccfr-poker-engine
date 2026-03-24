mkdir -p build && cd build
cmake -G Ninja ..
ninja poker_tests
./poker_tests
