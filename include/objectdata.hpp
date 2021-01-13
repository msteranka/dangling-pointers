#ifndef __OBJECT_DATA_HPP
#define __OBJECT_DATA_HPP

#include "pin.H"
#include <iostream>
#include "backtrace.hpp"

struct ObjectData {
    ObjectData(ADDRINT addr, UINT32 size, THREADID mallocThread) : 
        addr(addr),
        size(size),
        isLive(true),
        mallocThread(mallocThread),
        freeThread(-1) { }

    // VOID SetMallocTrace(Backtrace &b) { mallocTrace = b; } // NOT THREAD-SAFE

    // VOID SetFreeTrace(CONTEXT *ctxt) { freeTrace.SetTrace(ctxt); } // NOT THREAD-SAFE

    ADDRINT addr;
    UINT32 size;
    BOOL isLive;
    THREADID mallocThread, freeThread;
    // Backtrace mallocTrace, freeTrace;
};

#endif
