#if !defined(__MEMSET_HPP)
# define __MEMSET_HPP

void my_memset(void *s, int c, size_t n) {
    for (int i = 0; i < n; i++) {
        *((char *) s + i) = c;
    }
}

#endif // __MEMSET_HPP
