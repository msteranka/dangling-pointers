#include <iostream>
#include <cstdlib>
#include "memset.hpp"

void region_begin() {
    std::cout << "In region_begin()" << std::endl;
}

void region_end() {
    std::cout << "In region_end()" << std::endl;
}

void *region_malloc(size_t size) {
    std::cout << "In region_malloc()" << std::endl;
    return malloc(size);
}

void region_free(void *ptr) {
    std::cout << "In region_free()" << std::endl;
    free(ptr);
}

int main() {
    region_begin();
    void *ptr = region_malloc(20);
    region_free(ptr);
    my_memset(ptr, 'a', 20);
    region_end();
    return 0;
}
