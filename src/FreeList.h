#pragma once

#include "Common.h"

class FreeList{
public:
	// 获取当前桶中有多少块空间
	size_t Size();

	// 删除掉桶中n个块（头删）
	void PopRange(void*& start, void*& end, size_t n);

	// 向自由链表中头插，且插入多块空间
	void PushRange(void* start, void* end, size_t n);

    // 判断哈希桶是否为空
	bool Empty();

    // 用来回收空间的
	void Push(void* obj);

    // 用来提供空间的
	void* Pop();

	// FreeList当前未到上限时，能够申请的最大块空间是多少
	size_t& MaxSize();

private:
    size_t _size;
    size_t _maxSize = 1;
    void* _freeList = nullptr;
};


