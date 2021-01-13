#include <cstdlib>
#include <cassert>

int main() {
    char *buf = (char *) malloc(20);
    assert(buf != nullptr);
    free(buf);
    buf[0] = 'a';
    return 0;
}
