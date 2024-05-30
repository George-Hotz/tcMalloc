#include "FreeList.h"


// 获取当前桶中有多少块空间
size_t FreeList::Size(){
    return _size;
}

// 删除掉桶中n个块（头删）
void FreeList::PopRange(void*& start, void*& end, size_t n){
    assert(n <= _size);
    start = end = _freeList;

    for (size_t i = 0; i < n - 1; ++i){
        end = ObjNext(end);
    }

    _freeList = ObjNext(end);
    ObjNext(end) = nullptr;
    _size -= n;
}

// 向自由链表中头插，且插入多块空间
void FreeList::PushRange(void* start, void* end, size_t n){
    ObjNext(end) = _freeList;
    _freeList = start;
    _size += n;
}

// 判断哈希桶是否为空
bool FreeList::Empty(){ 
    return _freeList == nullptr;
}

// 用来回收空间的
void FreeList::Push(void* obj){ 
    assert(obj); 
    ObjNext(obj) = _freeList;
    _freeList = obj;
    ++_size; 
}

// 用来提供空间的
void* FreeList::Pop(){ 
    assert(_freeList); 
    void* obj = _freeList;
    _freeList = ObjNext(obj);
    --_size; 
    return obj;
}

// FreeList当前未到上限时，能够申请的最大块空间是多少
size_t& FreeList::MaxSize(){
    return _maxSize;
}

