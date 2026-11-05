// g++ -std=c++17 -Wall -I src/AyonUsdResolver/cache tests/memcachedChunksTest.cpp -o /tmp/t2 && /tmp/t2
#include "memcachedChunks.h"

#include <cassert>
#include <iostream>

using namespace MemcachedChunks;

int main() {
    assert(chunkCount(0) == 0);
    assert(chunkCount(1) == 1);
    assert(chunkCount(256) == 1);
    assert(chunkCount(257) == 2);
    assert(chunkCount(512) == 2);
    assert(chunkCount(513) == 3);
    assert(chunkCount(10, 0) == 0);
    std::cout << "ok\n";
}
