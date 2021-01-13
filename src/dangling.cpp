#include "pin.H"
#include <fstream>
#include <iostream>
#include <utility>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include "objectdata.hpp"
#include "backtrace.hpp"
#include "objectmanager.hpp"
#include "mytls.hpp"

#ifdef TARGET_MAC
#define MALLOC "_malloc"
#define FREE "_free"
#else
#define MALLOC "malloc"
#define FREE "free"
#endif // TARGET_MAC

using namespace std;

static ObjectManager manager;
static TLS_KEY tls_key = INVALID_TLS_KEY; // Thread Local Storage
static PIN_LOCK outputLock;

VOID ThreadStart(THREADID threadId, CONTEXT *ctxt, INT32 flags, VOID* v) {
    MyTLS *tls = new MyTLS;
    assert(PIN_SetThreadData(tls_key, tls, threadId));
}

VOID ThreadFini(THREADID threadId, const CONTEXT *ctxt, INT32 code, VOID* v) {
    MyTLS *tls = static_cast<MyTLS*>(PIN_GetThreadData(tls_key, threadId));
    delete tls;
}

// Function arguments and backtrace can only be accessed at the function entry point
// Thus, we must insert a routine before malloc and cache these values
//
VOID MallocBefore(THREADID threadId, CONTEXT *ctxt, ADDRINT size) {
    MyTLS *tls = static_cast<MyTLS*>(PIN_GetThreadData(tls_key, threadId));
    // threadCache->second.SetTrace(ctxt);
    tls->_cachedSize = size;
}

VOID MallocAfter(THREADID threadId, ADDRINT retVal) {
    // Check for success since we don't want to track null pointers
    //
    if ((VOID *) retVal == nullptr) { 
        return; 
    }

    MyTLS *tls = static_cast<MyTLS*>(PIN_GetThreadData(tls_key, threadId));
    // UINT32 size = threadCache->first;
    // Backtrace b = threadCache->second;
    manager.InsertObject(retVal, tls->_cachedSize, tls->_cachedBacktrace, threadId);
}

VOID FreeBefore(THREADID threadId, CONTEXT *ctxt, ADDRINT ptr) {
    MyTLS *tls = static_cast<MyTLS*>(PIN_GetThreadData(tls_key, threadId));
    tls->_cachedPtr = (void *) ptr;
    // tls->_cachedBacktrace.SetTrace(ctxt);
}

// We must separate FreeBefore and FreeAfter to avoid considering any metadata
// changes within the allocator as use-after-frees
//
VOID FreeAfter(THREADID threadId) {
    MyTLS *tls = static_cast<MyTLS*>(PIN_GetThreadData(tls_key, threadId));
    manager.DeleteObject((ADDRINT) tls->_cachedPtr, tls->_cachedBacktrace, threadId);
}

VOID ReadsMem(THREADID threadId, ADDRINT addrRead, UINT32 readSize) {
    ObjectData *d = manager.IsUseAfterFree(addrRead, readSize, threadId);
    if (d == nullptr) {
        return;
    }
    PIN_GetLock(&outputLock, threadId);
    std::cout << "Read " << readSize << " byte(s) at address <" << std::hex << 
        d->addr << std::dec << "+" << addrRead - d->addr << ">" << std::endl;
    PIN_ReleaseLock(&outputLock);
}

VOID WritesMem(THREADID threadId, ADDRINT addrWritten, UINT32 writeSize) {
    ObjectData *d = manager.IsUseAfterFree(addrWritten, writeSize, threadId);
    if (d == nullptr) {
        return;
    }
    PIN_GetLock(&outputLock, threadId);
    std::cout << "Wrote " << writeSize << " byte(s) at address <" << std::hex << 
        d->addr << std::dec << "+" << addrWritten - d->addr << ">" << std::endl;
    PIN_ReleaseLock(&outputLock);
}

VOID Instruction(INS ins, VOID *v) 
{
    if (INS_IsMemoryRead(ins) && !INS_IsStackRead(ins)) 
    {
        // Intercept read instructions that don't read from the stack with ReadsMem
        //
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) ReadsMem,
                        IARG_THREAD_ID,
                        IARG_MEMORYREAD_EA,
                        IARG_MEMORYREAD_SIZE,
                        IARG_END);
    }

    if (INS_IsMemoryWrite(ins) && !INS_IsStackWrite(ins)) 
    {
        // Intercept write instructions that don't write to the stack with WritesMem
        //
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) WritesMem,
                        IARG_THREAD_ID,
                        IARG_MEMORYWRITE_EA,
                        IARG_MEMORYWRITE_SIZE,
                        IARG_END);
    }
}

VOID Image(IMG img, VOID *v) 
{
    RTN rtn;

    rtn = RTN_FindByName(img, MALLOC);
    if (RTN_Valid(rtn)) 
    {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR) MallocBefore, // Hook calls to malloc with MallocBefore
                        IARG_THREAD_ID,
                        IARG_CONST_CONTEXT,
                        IARG_FUNCARG_ENTRYPOINT_VALUE, 
                        0, IARG_END);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR) MallocAfter, // Hook calls to malloc with MallocAfter
                        IARG_THREAD_ID,
                        IARG_FUNCRET_EXITPOINT_VALUE, 
                        IARG_END);
        RTN_Close(rtn);
    }

    rtn = RTN_FindByName(img, FREE);
    if (RTN_Valid(rtn)) 
    {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR) FreeBefore, // Hook calls to free with FreeBefore
                        IARG_THREAD_ID,
                        IARG_CONST_CONTEXT, 
                        IARG_FUNCARG_ENTRYPOINT_VALUE, 
                        0, IARG_END);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR) FreeAfter, // Hook calls to free with FreeAfter
                        IARG_THREAD_ID,
                        IARG_END);
        RTN_Close(rtn);
    }
}

INT32 Usage() {
    return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    PIN_InitSymbols();

    if (PIN_Init(argc, argv))  {
        return Usage();
    }

    tls_key = PIN_CreateThreadDataKey(NULL);
    PIN_InitLock(&outputLock);
    if (tls_key == INVALID_TLS_KEY)
    {
        cerr << "number of already allocated keys reached the MAX_CLIENT_TLS_KEYS limit" << endl;
        PIN_ExitProcess(1);
    }

    IMG_AddInstrumentFunction(Image, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);
    PIN_StartProgram();
}
