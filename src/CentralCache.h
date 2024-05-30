#pragma once

#include "Span.h"
#include "SizeClass.h"
#include "Fixed_Pool.h"
#include "FreeList.h"
#include "ThreadCache.h"
#include "CentralCache.h"
#include "PageCache.h"
#include "PageMap.h"
#include "Common.h"

class CentralCache{
public:
    // 单例饿汉
    static CentralCache* GetInstance(){
        static CentralCache _sInst;  
        return &_sInst;
    }

    // cc从自己的_spanLists中为tc提供tc所需要的块空间
    size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);

    // 获取一个管理空间不为空的span
    Span* GetOneSpan(SpanList& list, size_t size);

    // 将tc还回来的多块空间放到span中
    void ReleaseListToSpans(void* start, size_t size);

private:
    // 单例，去掉构造、拷构和拷赋
    CentralCache() {}
    CentralCache(const CentralCache&) = delete;
    CentralCache& operator=(const CentralCache&) = delete;

private:
    SpanList _spanLists[Free_List_Num]; // 哈希桶中挂的是一个一个的Span
	
};