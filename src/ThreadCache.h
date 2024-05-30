#pragma once

#include "Common.h"
#include "FreeList.h"
#include"CentralCache.h"

class ThreadCache{
public:
    static ThreadCache* Get_Instance() {
        thread_local ThreadCache pTLS_tc;
        return &pTLS_tc;
    }
    
    void* Alloc_tc(size_t size);
    void Free_tc(void *obj, size_t size);

    void* Fetch_cc(size_t index, size_t alignSize);
    void Restore_cc(FreeList& list, size_t size);

private:
    FreeList _freeLists[Free_List_Num];
};



