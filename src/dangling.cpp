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

#if defined(_MSC_VER)
# define LIKELY(x) (x)
# define UNLIKELY(x) (x)
#else
# define LIKELY(x) __builtin_expect(!!(x), 1)
# define UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif // _MSC_VER

#if defined(TARGET_MAC)
# define MALLOC "_malloc"
# define FREE "_free"
#else
# define MALLOC "malloc"
# define FREE "free"
#endif // TARGET_MAC

using namespace std;

static ObjectManager manager;
static TLS_KEY tls_key = INVALID_TLS_KEY; // Thread Local Storage
static PIN_LOCK outputLock;

namespace Params {
    static const std::string defaultIsVerbose = "0";
    static BOOL isVerbose;
};

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
    tls->_cachedSize = size;
    tls->_cachedBacktrace.SetTrace(ctxt);
    tls->_inMalloc = true;
}

VOID MallocAfter(THREADID threadId, ADDRINT retVal) {
    // Check for success since we don't want to track null pointers
    //
    if ((VOID *) retVal == nullptr) { 
        return; 
    }

    MyTLS *tls = static_cast<MyTLS*>(PIN_GetThreadData(tls_key, threadId));
    manager.InsertObject(retVal, tls->_cachedSize, tls->_cachedBacktrace, threadId);
    tls->_inMalloc = false;
}

VOID FreeBefore(THREADID threadId, CONTEXT *ctxt, ADDRINT ptr) {
    MyTLS *tls = static_cast<MyTLS*>(PIN_GetThreadData(tls_key, threadId));
    tls->_cachedPtr = (void *) ptr;
    tls->_cachedBacktrace.SetTrace(ctxt);
}

// We must separate FreeBefore and FreeAfter to avoid considering any metadata
// changes within the allocator as use-after-frees
//
VOID FreeAfter(THREADID threadId) {
    MyTLS *tls = static_cast<MyTLS*>(PIN_GetThreadData(tls_key, threadId));
    manager.DeleteObject((ADDRINT) tls->_cachedPtr, tls->_cachedBacktrace, threadId);
}

VOID PrintUseAfterFree(ObjectData *d, THREADID accessingThread, ADDRINT addrAccessed, UINT32 accessSize, const CONTEXT *ctxt) {
    std::string fileName;
    INT32 lineNumber;
    ADDRINT ip;

    ip = PIN_GetContextReg(ctxt, REG_INST_PTR);
    PIN_LockClient();
    PIN_GetSourceLocation(ip, nullptr, &lineNumber, &fileName);
    PIN_UnlockClient();

    PIN_GetLock(&outputLock, accessingThread);
    std::cout << "Thread " << accessingThread << " accessed " << accessSize << " byte(s) at address <" << std::hex << 
        d->_addr << std::dec << "+" << addrAccessed - d->_addr << ">" << " in " << fileName << ":" << lineNumber << std::endl;
    if (Params::isVerbose) {
        std::cout << "\tAllocated by thread " << d->_mallocThread << " @" << std::endl << d->_mallocTrace << 
            "\tFreed by thread " << d->_freeThread << " @" << std::endl << d->_freeTrace;
    }
    PIN_ReleaseLock(&outputLock);
}

VOID ReadsMem(THREADID threadId, ADDRINT addrRead, UINT32 readSize, const CONTEXT *ctxt) {
    MyTLS *tls = static_cast<MyTLS*>(PIN_GetThreadData(tls_key, threadId));
    ObjectData *d = manager.IsUseAfterFree(addrRead, readSize, threadId);
    if (UNLIKELY(tls->_inMalloc)) { // If this is a read during malloc
        return;
    }
    if (LIKELY(!d)) { // If this is a valid read
        return;
    }
    PrintUseAfterFree(d, threadId, addrRead, readSize, ctxt);
}

VOID WritesMem(THREADID threadId, ADDRINT addrWritten, UINT32 writeSize, const CONTEXT *ctxt) {
    MyTLS *tls = static_cast<MyTLS*>(PIN_GetThreadData(tls_key, threadId));
    ObjectData *d = manager.IsUseAfterFree(addrWritten, writeSize, threadId);
    if (UNLIKELY(tls->_inMalloc)) { // If this is a write during malloc
        return;
    }
    if (LIKELY(!d)) { // If this is a valid write
        return;
    }
    PrintUseAfterFree(d, threadId, addrWritten, writeSize, ctxt);
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
                        IARG_CONST_CONTEXT,
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
                        IARG_CONST_CONTEXT,
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
    KNOB<UINT32> knobIsVerbose(KNOB_MODE_WRITEONCE, "pintool", "v", 
                            Params::defaultIsVerbose,
                            "Dispay additional information including backtraces");

    if (PIN_Init(argc, argv))  {
        return Usage();
    }

    Params::isVerbose = knobIsVerbose.Value();
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
