#include"ThreadCache.h"


// 线程向tc申请size大小的空间
void* ThreadCache::Alloc_tc(size_t size){

    assert(size <= MAX_BYTES); 

    size_t alignSize = SizeClass::RoundUp(size); 
    size_t index = SizeClass::Index(size);

    if (!_freeLists[index].Empty()){   
        // 自由链表中不为空，可以直接从自由链表中获取空间
        return _freeLists[index].Pop();
    }else{ 
        // 自由链表为空，ThreadCache 向 CentralCache 申请空间
        return Fetch_cc(index, alignSize); 
    }
}

// 回收线程中大小为size的obj空间
void ThreadCache::Free_tc(void* obj, size_t size){

    assert(obj);
    assert(size <= MAX_BYTES); 

    size_t index = SizeClass::Index(size);
    _freeLists[index].Push(obj);  // 用对应自由链表回收空间

    // 当前桶中的块数大于等于单批次申请块数的时候归还空间
    if (_freeLists[index].Size() >= _freeLists[index].MaxSize()){
        Restore_cc(_freeLists[index], size);
    }
}

// ThreadCache中空间不够时，向CentralCache申请空间的接口
void* ThreadCache::Fetch_cc(size_t index, size_t alignSize){

	// 通过MaxSize和NumMoveSie来控制当前给tc提供多少块alignSize大小的空间
	size_t batchNum = std::min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(alignSize));

	if(batchNum == _freeLists[index].MaxSize()){ 
        //如果没有到达上限，那下次再申请这块空间的时候可以多申请一块
        _freeLists[index].MaxSize()++; 
        // 这里就是慢开始反馈调节的核心
	}

    // 输出型参数，返回之后的结果就是tc想要的空间
    void* start = nullptr;
    void* end = nullptr;

    // 返回值为实际获取到的块数
    size_t actulNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, alignSize);

    assert(actulNum >= 1); //actualNum一定是大于等于1的，这是FetchRangeObj能保证的

    if (actulNum == 1)
    { 
        // 如果actulNum等于1，就直接将start返回给线程
        assert(start == end);
        return start;
    }else{ 
        // 如果actulNum大于1，就还要给tc对应位置插入[ObjNext(start), end]的空间
        _freeLists[index].PushRange(ObjNext(start), end, actulNum - 1);
        // 给线程返回start所指空间
        return start;
    }
}

// tc向cc归还空间
void ThreadCache::Restore_cc(FreeList& list, size_t size){ 
    void* start = nullptr;
    void* end = nullptr;

    // 获取MaxSize块空间
    list.PopRange(start, end, list.MaxSize());

    // 归还空间
    CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}
