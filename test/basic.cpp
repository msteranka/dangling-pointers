#include <iostream>
#include <cstdlib>
#include <cassert>

void read_all(void *s, size_t n) {
    int c;
    for (int i = 0; i < n; i++) {
        c = *((char *) s + i);
    }
}

void write_all(void *s, int c, size_t n) {
    for (int i = 0; i < n; i++) {
        *((char *) s + i) = c;
    }
}

int main() {
    const unsigned int SIZE = 20;
    char *buf = (char *) malloc(SIZE);
    assert(buf != nullptr);

    std::cout << "APPLICATION: BEGINNING VALID WRITES" << std::endl;
    write_all(buf, 'a', SIZE);
    std::cout << "APPLICATION: ENDING VALID WRITES" << std::endl;

    std::cout << "APPLICATION: BEGINNING VALID READS" << std::endl;
    read_all(buf, SIZE);
    std::cout << "APPLICATION: ENDING VALID READS" << std::endl;

    free(buf);

    std::cout << "APPLICATION: BEGINNING INVALID WRITES" << std::endl;
    write_all(buf, 'a', SIZE);
    std::cout << "APPLICATION: ENDING INVALID WRITES" << std::endl;

    std::cout << "APPLICATION: BEGINNING INVALID READS" << std::endl;
    read_all(buf, SIZE);
    std::cout << "APPLICATION: ENDING INVALID READS" << std::endl;

    return 0;
}
