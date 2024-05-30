#pragma once

#include "Common.h"
#include "SizeClass.h"
#include "ThreadCache.h"
#include "PageCache.h"
#include "Span.h"

/*
    给线程提供的申请内存和释放内存的接口
*/


void* tc_malloc(size_t size){

    if (size <= MAX_BYTES){
        // TLS线程间相互独立
        return ThreadCache::Get_Instance()->Alloc_tc(size);

    }else{
        size_t alignSize = SizeClass::RoundUp(size);        // 先按照页大小对齐
        size_t k = alignSize >> PAGE_SHIFT;                 // 算出来对齐之后需要多少页

        PageCache::GetInstance()->_pageMtx.lock();          // 对pc中的span进行操作，加锁
        Span* span = PageCache::GetInstance()->NewSpan(k);  // 直接向pc要
        span->_objSize = size;                              // 统计大于256KB的页
        PageCache::GetInstance()->_pageMtx.unlock();        // 解锁

        void* ptr = (void*)(span->_pageID << PAGE_SHIFT);   // 通过获得到的span来提供空间
        return ptr;
    }


}


void tc_free(void *ptr){
    assert(ptr);

    // 通过ptr找到对应的span，因为前面申请空间的
    // 时候已经保证了维护的空间首页地址已经映射过了
    Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
    size_t size = span->_objSize; // 通过映射来的span获取ptr所指空间大小

    // 通过size判断是不是大于256KB的，是了就走pc
    if (size > MAX_BYTES){
        PageCache::GetInstance()->_pageMtx.lock(); // 记得加锁解锁
        PageCache::GetInstance()->ReleaseSpanToPageCache(span); // 直接通过span释放空间
        PageCache::GetInstance()->_pageMtx.unlock(); // 记得加锁解锁
    }else{
        // 不是大于256KB的就走tc
        ThreadCache::Get_Instance()->Free_tc(ptr, size);
    }
}