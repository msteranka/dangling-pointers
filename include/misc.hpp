#if !defined(__MISC_HPP)
# define __MISC_HPP

#include "pin.H"
#include <iostream>
#include <sstream>
#include "objectdata.hpp"

VOID GetSource(std::ostringstream &source, const CONTEXT *ctxt) {
    std::string fileName;
    INT32 lineNumber;
    ADDRINT ip;

    ip = PIN_GetContextReg(ctxt, REG_INST_PTR);
    PIN_LockClient();
    PIN_GetSourceLocation(ip, nullptr, &lineNumber, &fileName);
    PIN_UnlockClient();

    source << fileName << ":" << lineNumber;
}

std::ostream &PrintUseAfterFree(std::ostream &os, ObjectData *d, THREADID accessingThread, ADDRINT addrAccessed, UINT32 accessSize, std::string &source, PIN_LOCK &outputLock) {
    PIN_GetLock(&outputLock, accessingThread);
    os << "Thread " << accessingThread << " accessed " << accessSize << " byte(s) at address <" << std::hex << 
        d->_addr << std::dec << "+" << addrAccessed - d->_addr << ">" << " in " << source << std::endl <<
        "\tAllocated by thread " << d->_mallocThread << " @" << std::endl << d->_mallocTrace << 
        "\tFreed by thread " << d->_freeThread << " @" << std::endl << d->_freeTrace;
    PIN_ReleaseLock(&outputLock);
    return os;
}

#endif // __MISC_HPP
