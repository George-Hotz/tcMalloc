#pragma once

#include "SizeClass.h"
#include "Common.h"

// Single-level array

template <int BITS>          // 表示所有页号所需要的位数
class TCMalloc_PageMap1 {
private:
    static const int LENGTH = 1 << BITS;
    void** array_;

public:
    typedef uintptr_t Number;

    explicit TCMalloc_PageMap1() {
        
        // array_中存放的都是span*，也就是存放的都是指针，每个页号对应一个span*，也就是(总页数 * 指针大小)
        // 假设这里是32位的，4左移19，等价于4左移6位再左移13位，也就是256左移13位，也就是256页
        size_t size = sizeof(void*) << BITS;
        
        // 那么这里在对其size的时候，size是大于256KB的，就会按照一页进行对齐，不过size已经是256页了，已
        // 经是对齐的数了，所以size是等于alignsize的
        size_t alignSize = SizeClass::_RoundUp(size, 1 << PAGE_SHIFT);
        
        //cout << size << ' ' << alignSize << endl;

        // 这里数组要直接开256页，也就是256 * 8KB，即2048KB，也就是2MB
        array_ = (void**)SystemAlloc(alignSize >> PAGE_SHIFT);
        memset(array_, 0, sizeof(void*) << BITS);
    }

    // Return the current value for KEY.  Returns NULL if not yet set,
    // or if k is out of range.
    void* get(Number k) const {
        if ((k >> BITS) > 0)
        {	// 通过页号找对应的span，如果页号位数超过19，那就是不可能的情况
            return NULL;
        }
        return array_[k];
    }

    // REQUIRES "k" is in range "[0,2^BITS-1]".
    // REQUIRES "k" has been ensured before.
    //
    // Sets the value 'v' for key 'k'.
    void set(Number k, void* v) {
        array_[k] = v;	// 将页号设置为对应span
    }
};