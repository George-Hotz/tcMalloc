#pragma once

#include "Common.h"

// 以页为基本单位的结构体
struct Span{
public:
    PageID _pageID = 0;                  // 页号
    size_t _n = 0;                       // 当前span管理的页的数量
    size_t _objSize = 0;                 // span管理页被切分成的块有多大

    void* _freeList = nullptr;           // 每个span下面挂的小块空间的头结点
    size_t use_count = 0;                // 当前span分配出去了多少个块空间

    Span* _prev = nullptr;               // 前一个节点
    Span* _next = nullptr;               // 后一个节点

    bool _isUse = false;                 // 判断当前span是在cc中还是在pc中
};


class SpanList{
public:
    SpanList();

    // 判空
    bool Empty();

    // 头结点
    Span* Begin();

    // 尾结点
    Span* End();
    
    void Insert(Span* pos, Span* ptr);

    void Erase(Span* pos);

    void PushFront(Span* span);

    Span* PopFront();

private:
	Span* _head;                         // 哨兵位头结点
public:
	std::mutex _mtx;                     //桶锁
};