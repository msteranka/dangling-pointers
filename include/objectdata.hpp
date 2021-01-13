#ifndef __OBJECT_DATA_HPP
#define __OBJECT_DATA_HPP

#include "pin.H"
#include <iostream>
#include "backtrace.hpp"

struct ObjectData {
    ObjectData(ADDRINT addr, UINT32 size, THREADID mallocThread, Backtrace mallocTrace) : 
        _addr(addr),
        _size(size),
        _isLive(true),
        _mallocThread(mallocThread),
        _freeThread(-1) { 
            _mallocTrace = mallocTrace;
            }

    // VOID SetMallocTrace(Backtrace &b) { mallocTrace = b; } // NOT THREAD-SAFE

    // VOID SetFreeTrace(CONTEXT *ctxt) { freeTrace.SetTrace(ctxt); } // NOT THREAD-SAFE

    ADDRINT _addr;
    UINT32 _size;
    BOOL _isLive;
    THREADID _mallocThread, _freeThread;
    Backtrace _mallocTrace, _freeTrace;
};

#endif
