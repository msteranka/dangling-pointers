#include <iostream>
#include <cstdlib>
#include <cassert>

void my_memset(void *s, int c, size_t n) {
    for (int i = 0; i < n; i++) {
        *((char *) s + i) = c;
    }
}

void smallToBig(const unsigned int smallSize, const unsigned int largeSize) {
    char *ptr1 = (char *) malloc(smallSize);
    my_memset(ptr1, 'a', smallSize);
    free(ptr1);
    my_memset(ptr1, 'a', smallSize);
    std::cout << "---------------------------------------------" << std::endl;
    char *ptr2 = (char *) malloc(largeSize);
    assert(ptr1 == ptr2);
    my_memset(ptr2, 'a', largeSize);
    free(ptr2);
    my_memset(ptr2, 'a', largeSize);
}

void bigToSmall(const unsigned int largeSize, const unsigned int smallSize) {
    char *ptr1 = (char *) malloc(largeSize);
    my_memset(ptr1, 'a', largeSize);
    free(ptr1);
    my_memset(ptr1, 'a', largeSize);
    std::cout << "---------------------------------------------" << std::endl;
    char *ptr2 = (char *) malloc(smallSize);
    assert(ptr1 == ptr2);
    my_memset(ptr2, 'a', smallSize);
    free(ptr2);
    my_memset(ptr2, 'a', smallSize);
}

int main() {
    const unsigned int smallSize = 16, largeSize = 24;
    smallToBig(smallSize, largeSize);
    std::cout << "---------------------------------------------" << std::endl;
    bigToSmall(largeSize, smallSize);
    return 0;
}
