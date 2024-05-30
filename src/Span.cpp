#include "Span.h"


SpanList::SpanList(){ 
    _head = new Span;                // 构造函数中搞哨兵位头结点
    _head->_next = _head;            // 因为是双向循环的，所以都指向_head
    _head->_prev = _head;
}

// 判空
bool SpanList::Empty(){                        
    return _head == _head->_next;    // 带头双向循环空的时候_head指向自己
}

// 头结点
Span* SpanList::Begin(){                        
    return _head->_next;
}

// 尾结点
Span* SpanList::End(){                          
    return _head;
}

void SpanList::Insert(Span* pos, Span* ptr){ 
    // 在pos前面插入ptr
    assert(pos); // pos不为空
    assert(ptr); // ptr不为空

    Span* prev = pos->_prev;          

    prev->_next = ptr;
    ptr->_prev = prev;

    ptr->_next = pos;
    pos->_prev = ptr;
}

void SpanList::Erase(Span* pos){
    assert(pos);                      // pos不为空
    assert(pos != _head);             // pos不能是哨兵位

    Span* prev = pos->_prev;
    Span* next = pos->_next;

    prev->_next = next;
    next->_prev = prev;

    /*pos节点不需要调用delete删除，因为
    pos节点的Span需要回收，而不是直接删掉*/
    // 回收相关逻辑
}

void SpanList::PushFront(Span* span){          // 头插
    Insert(Begin(), span);
}

Span* SpanList::PopFront(){          // 使用掉第一个span
    Span* front = _head->_next;      // 先获取到_head后面的第一个span
    Erase(front);                    // 删除掉这个span，直接复用Erase
    return front;                    // 返回原来的第一个span
}