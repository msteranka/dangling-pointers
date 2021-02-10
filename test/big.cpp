#include <cstdlib>
#include <cassert>
#include "memset.hpp"

int main() {
    void *ptr = malloc(100);
    assert(ptr != nullptr);
    free(ptr);
    for (int i = 0; i < 100; i++) {
        my_memset(ptr, 'a', 100);
    }
    return 0;
}
