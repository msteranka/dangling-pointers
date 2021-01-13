#if !defined(__MY_TLS_HPP)
# define __MY_TLS_HPP

#include "backtrace.hpp"

struct MyTLS {
    MyTLS() { }

    void *_cachedPtr;
    size_t _cachedSize;
    Backtrace _cachedBacktrace;
};

#endif // __MY_TLS_HPP
