#ifndef __OBJECT_MANAGER_HPP
#define __OBJECT_MANAGER_HPP

#include "pin.H"
#include "backtrace.hpp"
#include <unordered_map>
#include <vector>

using namespace std;

// All of ObjectManager's methods are thread-safe unless specified otherwise
//
class ObjectManager {
    public:
        ObjectManager() {
            PIN_InitLock(&allObjectsLock);
        }

        VOID InsertObject(ADDRINT ptr, UINT32 size, Backtrace trace, THREADID threadId) {
            unordered_map<ADDRINT,ObjectData*>::iterator it;
            ObjectData *d;
            ADDRINT nextAddr;

            // If this object has already been inserted before, delete the
            // previous object data it mapped to
            //
            PIN_GetLock(&allObjectsLock, threadId);
            it = allObjects.find(ptr);
            if (it != allObjects.end()) { // If this object is already within allObjects, change its contents
                d = it->second;
                *d = ObjectData(ptr, size, threadId);
                // d->mallocTrace = trace;
                PIN_ReleaseLock(&allObjectsLock);
            } else { // If this object is not within allObjects, allocate a new ObjectData
                PIN_ReleaseLock(&allObjectsLock);
                d = new ObjectData(ptr, size, threadId);
                // d->mallocTrace = trace;
            }

            // Create a mapping from every address in this object's range to the same ObjectData
            //
            for (UINT32 i = 0; i < size; i++) {
                nextAddr = (ptr + i);
                PIN_GetLock(&allObjectsLock, threadId);
                allObjects[nextAddr] = d;
                // allObjects.insert(make_pair<ADDRINT,ObjectData*>(nextAddr, d));
                PIN_ReleaseLock(&allObjectsLock);
            }
        }

        VOID DeleteObject(ADDRINT ptr, Backtrace trace, THREADID threadId)
        {
            unordered_map<ADDRINT,ObjectData*>::iterator it;
            ObjectData *d;

            // Determine if this is an invalid/double free, and if it is, then 
            // skip this routine
            //
            PIN_GetLock(&allObjectsLock, threadId);
            it = allObjects.find(ptr);
            if (it == allObjects.end()) {
                PIN_ReleaseLock(&allObjectsLock);
                return;
            }
            PIN_ReleaseLock(&allObjectsLock);
    
            // Update object metadata, also marking the object as no longer live
            //
            d = it->second;
            d->freeThread = threadId;
            // d->freeTrace = trace;
            d->isLive = false;
        }

        ObjectData *IsUseAfterFree(ADDRINT addr, UINT32 size, THREADID threadId) {
            unordered_map<ADDRINT,ObjectData*>::iterator it;
            ObjectData *d;

            // Determine whether addr corresponds to an object returned by malloc, 
            // and if it isn't, then skip this routine
            //
            PIN_GetLock(&allObjectsLock, threadId);
            it = allObjects.find(addr);
            if (it == allObjects.end()) {
                PIN_ReleaseLock(&allObjectsLock);
                return nullptr;
            }
            PIN_ReleaseLock(&allObjectsLock);

            // TODO: Need atomicity for accessing object data?
            //
            d = it->second;
            if (d->isLive) {
                return nullptr;
            } else {
                return d;
            }
        }

    private:
        unordered_map<ADDRINT,ObjectData*> allObjects;
        PIN_LOCK allObjectsLock;
};

#endif
