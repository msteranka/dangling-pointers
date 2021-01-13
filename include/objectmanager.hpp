#ifndef __OBJECT_MANAGER_HPP
#define __OBJECT_MANAGER_HPP

#include "pin.H"
#include "backtrace.hpp"
#include <unordered_map>

using namespace std;

// All of ObjectManager's methods are thread-safe unless specified otherwise
//
class ObjectManager {
    public:
        ObjectManager() {
            PIN_InitLock(&_allObjectsLock);
        }

        VOID InsertObject(ADDRINT ptr, UINT32 size, Backtrace trace, THREADID threadId) {
            unordered_map<ADDRINT,ObjectData*>::iterator it;
            ObjectData *d;
            ADDRINT nextAddr;

            // If this object has already been inserted before, delete the
            // previous object data it mapped to
            //
            PIN_GetLock(&_allObjectsLock, threadId);
            it = _allObjects.find(ptr);
            if (it != _allObjects.end()) { // If this object is already within _allObjects, change its contents
                PIN_ReleaseLock(&_allObjectsLock);
                d = it->second;
                *d = ObjectData(ptr, size, threadId, trace);
            } else { // If this object is not within _allObjects, allocate a new ObjectData
                PIN_ReleaseLock(&_allObjectsLock);
                d = new ObjectData(ptr, size, threadId, trace);
            }

            // Create a mapping from every address in this object's range to the same ObjectData
            //
            for (UINT32 i = 0; i < size; i++) {
                nextAddr = (ptr + i);
                PIN_GetLock(&_allObjectsLock, threadId);
                _allObjects[nextAddr] = d;
                // _allObjects.insert(make_pair<ADDRINT,ObjectData*>(nextAddr, d));
                PIN_ReleaseLock(&_allObjectsLock);
            }
        }

        VOID DeleteObject(ADDRINT ptr, Backtrace trace, THREADID threadId)
        {
            unordered_map<ADDRINT,ObjectData*>::iterator it;
            ObjectData *d;

            // Determine if this is an invalid/double free, and if it is, then 
            // skip this routine
            //
            PIN_GetLock(&_allObjectsLock, threadId);
            it = _allObjects.find(ptr);
            if (it == _allObjects.end()) {
                PIN_ReleaseLock(&_allObjectsLock);
                return;
            }
            PIN_ReleaseLock(&_allObjectsLock);
    
            // Update object metadata, also marking the object as no longer live
            //
            d = it->second;
            d->_freeThread = threadId;
            d->_freeTrace = trace;
            d->_isLive = false;
        }

        ObjectData *IsUseAfterFree(ADDRINT addr, UINT32 size, THREADID threadId) {
            unordered_map<ADDRINT,ObjectData*>::iterator it;
            ObjectData *d;

            // Determine whether addr corresponds to an object returned by malloc, 
            // and if it isn't, then skip this routine
            //
            PIN_GetLock(&_allObjectsLock, threadId);
            it = _allObjects.find(addr);
            if (it == _allObjects.end()) {
                PIN_ReleaseLock(&_allObjectsLock);
                return nullptr;
            }
            PIN_ReleaseLock(&_allObjectsLock);

            // TODO: Need atomicity for accessing object data?
            //
            d = it->second;
            if (d->_isLive) {
                return nullptr;
            } else {
                return d;
            }
        }

    private:
        unordered_map<ADDRINT,ObjectData*> _allObjects;
        PIN_LOCK _allObjectsLock;
};

#endif
