#include"PageCache.h"


// pc从_spanLists中拿出来一个k页的span
Span* PageCache::NewSpan(size_t k)
{
    assert(k > 0);

    // 如果单次申请的页数超过128页时才需要向os申请，如果没有超过128页的还可以向pc申请
    if (k > PAGE_NUM - 1){
        void* ptr = SystemAlloc(k);     // 直接向os申请
        Span* span = _spanPool.New();   // 用定长内存池开空间
        
        span->_pageID = ((PageID)ptr >> PAGE_SHIFT); // 申请空间的对应页号
        span->_n = k; // 申请了多少页
        
        // 把这个span管理的首页映射到哈希中，后面在删除这个span的时候能找到就行
        _idSpanMap[span->_pageID] = span;
        // 不需要把这个span让pc管理，pc只能管小于128页的span

        return span;
    }

    // (#1) k号桶中有span
    if (!_spanLists[k].Empty()){ 
        // 直接返回该桶中的第一个span
        Span* span = _spanLists[k].PopFront();

        // 记录分配出去的span管理的页号和其地址的映射关系
        for (PageID i = 0; i < span->_n; ++i){ 
            // n页的空间全部映射都是span地址
            _idSpanMap[span->_pageID + i] = span;
        }

        return span;
    }

    // (#2) k号桶没有span，但后面的桶中有span
    for (int i = k + 1; i < PAGE_NUM; ++i){ 
        // [k+1, PAGE_NUM - 1]号桶中有没有span
        if (!_spanLists[i].Empty()){ 
            // i号桶中有span，对该span进行切分
            
            // 获取到该桶中的span，起名就叫nSpan
            Span* nSpan = _spanLists[i].PopFront();

            // 将这个span切分成一个k页的和一个n-k页的span
            
            // Span的空间是需要新建的，而不是用当前内存池中的空间
            Span* kSpan = _spanPool.New(); // 用定长内存池开空间

            // 分一个k页的span
            kSpan->_pageID = nSpan->_pageID;
            kSpan->_n = k;

            // 和一个 n - k 页的span
            nSpan->_pageID += k;
            nSpan->_n -= k;

            // n - k页的放回对应哈希桶中
            _spanLists[nSpan->_n].PushFront(nSpan);

            // 再把n-k页的span边缘页映射一下，方便后续合并
            _idSpanMap[nSpan->_pageID] = nSpan;
            _idSpanMap[nSpan->_pageID + nSpan->_n - 1] = nSpan;

            // 记录分配出去的kSpan管理的页号和其地址的映射关系
            for (PageID i = 0; i < kSpan->_n; ++i){ 
                // n页的空间全部映射都是kSpan地址
                _idSpanMap[kSpan->_pageID + i] = kSpan;
            }

            return kSpan;
        }
    }

    // (#3) k号桶和后面的桶中都没有span

    // 直接向系统申请128页的span
    void* ptr = SystemAlloc(PAGE_NUM - 1); // PAGE_NUM为129
    Span* bigSpan = _spanPool.New(); // 用定长内存池开空间

    /* 只需要修改_pageID和_n即可,
    系统调用接口申请空间的时候一定能保证申请的空间是对齐的 */
    bigSpan->_pageID = ((PageID)ptr) >> PAGE_SHIFT;
    bigSpan->_n = PAGE_NUM - 1;

    // 将这个span放到对应哈希桶中
    _spanLists[PAGE_NUM - 1].PushFront(bigSpan);

    // 递归再次申请k页的span，这次递归一定会走②的逻辑
    return NewSpan(k);  // 复用代码
}

// 通过页地址找到span
Span* PageCache::MapObjectToSpan(void* obj){

    // 通过块地址找到页号
    PageID id = (((PageID)obj) >> PAGE_SHIFT);

    // 通过哈希找到页号对应span
    auto ret = _idSpanMap.find(id);

    // 这里的逻辑是一定能保证通过块地址找到一个span的，如果没找到就出错了
    if (ret != _idSpanMap.end()){
        return _idSpanMap[id];
    }else{
        assert(false);
        return nullptr;
    }
}

// 管理cc还回来的span
void PageCache::ReleaseSpanToPageCache(Span* span){

    // 通过span判断释放的空间页数是否大于128页，如果大于128页就直接还给os
    if (span->_n > PAGE_NUM - 1){
        void* ptr = (void*)(span->_pageID << PAGE_SHIFT); // 获取到要释放的地址
        SystemFree(ptr); // 直接调用系统接口释放空间
        //delete span; // 释放掉span
        _spanPool.Delete(span); // 用定长内存池删除span

        return;
    }

    /**************下面的都是原先的代码，也就是页数小于等于128页的span**************/
    // 向左不断合并
    while (1){
        PageID leftID = span->_pageID - 1; // 拿到左边相邻页
        auto ret = _idSpanMap.find(leftID);
        // 没有相邻span，停止合并
        if (ret == _idSpanMap.end()){
            break;
        }

        Span* leftSpan = _idSpanMap[leftID]; // 相邻span
        // 相邻span在cc中，停止合并
        if (leftSpan->_isUse == true){
            break;
        }

        // 相邻span与当前span合并后超过128页，停止合并
        if (leftSpan->_n + span->_n > PAGE_NUM - 1){
            break;
        }

        // 当前span与相邻span进行合并
        span->_pageID = leftSpan->_pageID;
        span->_n += leftSpan->_n;

        _spanLists[leftSpan->_n].Erase(leftSpan);// 将相邻span对象从桶中删掉
        //delete leftSpan;// 删除掉相邻span对象
        _spanPool.Delete(leftSpan); // 用定长内存池删除span
    }

    // 向右不断合并
    while (1){
        PageID rightID = span->_pageID + span->_n; // 右边的相邻页
        auto ret = _idSpanMap.find(rightID);

        // 没有相邻span，停止合并
        if (ret == _idSpanMap.end()){
            break;
        }

        Span* rightSpan = _idSpanMap[rightID]; // 右边的span
        // 相邻span在cc中，停止合并
        if (rightSpan->_isUse == true){
            break;
        }

        // 相邻span与当前span合并后超过128页，停止合并
        if (rightSpan->_n + span->_n > PAGE_NUM - 1){
            break;
        }

        // 当前span与相邻span进行合并
        span->_n += rightSpan->_n; // 往右边合并时不需要改span->_pageID，
                                    // 右边的会直接拼在span后面

        // 把桶里面的span删掉
        _spanLists[rightSpan->_n].Erase(rightSpan);
        _spanPool.Delete(rightSpan); // 用定长内存池删除span
    }

    // 合并完毕，将当前span挂到对应桶中
    _spanLists[span->_n].PushFront(span);
    span->_isUse = false; // 从cc返回到pc，isUse改成false

    // 映射当前span的边缘页，后续还可以对这个span合并
    _idSpanMap[span->_pageID] = span; 
    _idSpanMap[span->_pageID + span->_n - 1] = span;
}
