#pragma once

#include <cmath>
#include <mutex>
#include <atomic>
#include <vector>
#include <thread>
#include <cassert>
#include <iostream>
#include <unordered_map>

#include <string.h>

typedef size_t PageID;

static const size_t Free_List_Num = 208;           // 哈希桶中自由链表数
static const size_t MAX_BYTES = 256 * 1024;        // ThreadCache单次申请的最大字节数
static const size_t PAGE_SHIFT = 12;               // 一页多少位，这里给一页4KB，就是12位
static const size_t PAGE_NUM = 128 + 1;            // span的最大管理页数128，加1方便对齐

// 去堆上按页申请空间
inline static void* SystemAlloc(size_t kpage){
    void* ptr = (void*)malloc(kpage << PAGE_SHIFT);
	if (ptr == nullptr){
        throw std::bad_alloc();
    }
	return ptr;
}

// 直接去堆上释放空间
inline static void SystemFree(void* ptr){
    if(ptr == nullptr){
        return;
    }
    free(ptr);
}

inline static void*& ObjNext(void* obj){ 
	return *(void**)obj;
}