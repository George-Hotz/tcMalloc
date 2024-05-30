#pragma once

#include "Common.h"


template<class T>
class Fixed_Pool{
public:
    Fixed_Pool(size_t kpage);
    ~Fixed_Pool();

    T* New();
    void Delete(T *obj);

public:
    std::mutex _poolMtx; 

private:
    void *_memory = nullptr;       // 申请的过量内存块
    void *_freelist = nullptr;     // 空闲内存块链表
    size_t _remainBytes = 0;       // 内存池内存剩余的字节
    size_t _objSize = 0;           // 定长类型的大小
    size_t _pool_kpsize;           // 内存池的内存大小（以页为单位）
    std::vector<void*> _total;     // 保存申请的内存块(用于最后释放)
};


template<class T>
Fixed_Pool<T>::Fixed_Pool(size_t kpage)
    :_pool_kpsize(kpage), _remainBytes(kpage << PAGE_SHIFT){
    _objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
    _memory = SystemAlloc(_pool_kpsize);
    _total.push_back(_memory);
}


template<class T>
Fixed_Pool<T>::~Fixed_Pool(){
    for(auto it : _total){
        free(it);
    }
}


template<class T>
T* Fixed_Pool<T>::New(){
    T *obj = nullptr;

    if(_freelist){
        obj = (T*)_freelist;
        _freelist = ObjNext(_freelist);
    }else{
        if(_remainBytes < _objSize){
            _memory = SystemAlloc(_pool_kpsize);
            _total.push_back(_memory);
            _remainBytes = _pool_kpsize;
        }
        
        obj = (T*)_memory;
        _memory += _objSize;
        _remainBytes -= _objSize;
    }
    new(obj) T;   //定位new
    return obj;
}


template<class T>
void Fixed_Pool<T>::Delete(T *obj){
    obj->~T();    //显示调用析构

    ObjNext(obj) = _freelist;
    _freelist = obj;
}

