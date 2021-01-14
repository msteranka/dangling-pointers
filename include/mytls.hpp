#if !defined(__MY_TLS_HPP)
# define __MY_TLS_HPP

#include "backtrace.hpp"

struct MyTLS {
    MyTLS() : _inMalloc(false) { }

    void *_cachedPtr;
    size_t _cachedSize;
    Backtrace _cachedBacktrace;
    BOOL _inMalloc;
};

#endif // __MY_TLS_HPP
