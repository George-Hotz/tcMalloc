#pragma once

#include "Common.h"

class SizeClass{

    // 线程申请size的对齐规则：整体控制在最多10%左右的内碎片浪费
    //	size范围				对齐数				对应哈希桶下标范围
    // [1,128]					8B 对齐      		freelist[0,16)
    // [128+1,1024]			    16B 对齐  			freelist[16,72)
    // [1024+1,8*1024]			128B 对齐  			freelist[72,128)
    // [8*1024+1,64*1024]		1024B 对齐    		freelist[128,184)
    // [64*1024+1,256*1024]	    8*1024B 对齐  		freelist[184,208)
    
public:

    // 计算每个分区对应的对齐后的字节数
    static size_t _RoundUp(size_t size, size_t alignNum){
        return ((size + alignNum - 1) & ~(alignNum - 1));
    }

    // 计算对齐后的字节数，size为线程申请的空间大小
    static size_t RoundUp(size_t size) {
        if (size <= 128) {                 // [1,      128] 8B
            return _RoundUp(size, 8);
        }
        else if (size <= 1024) {           // [128+1,    K] 16B
            return _RoundUp(size, 16);
        }
        else if (size <= 8 * 1024){        // [1K+1,   8*K] 128B
            return _RoundUp(size, 128);
        }
        else if (size <= 64 * 1024){       // [8K+1,  64*K] 1KB
            return _RoundUp(size, 1024);
        }
        else if (size <= 256 * 1024){      // [64K+1, 256K] 8KB
            return _RoundUp(size, 8 * 1024);
        }
        else{                              //按照页来对齐
            return _RoundUp(size, 1 << PAGE_SHIFT);
        }
    }

    // 求size对应在哈希表中的下标（当前size所在区域的第几个下标，）
    static inline size_t _Index(size_t size, size_t align_shift){
        return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
    }

    // 计算映射的哪一个自由链表桶（tc和cc用，二者映射规则一样）
    static inline size_t Index(size_t size){
        assert(size <= MAX_BYTES);

        // 每个区间有多少个链
        static int group[4] = { 16, 56, 56, 56 };
        if (size <= 128){
            return _Index(size, 3); 
        }
        else if (size <= 1024){
            return _Index(size - 128, 4) + group[0];
        }
        else if (size <= 8 * 1024){
            return _Index(size - 1024, 7) + group[1] + group[0];
        }
        else if (size <= 64 * 1024){
            return _Index(size - 8 * 1024, 10) + group[2] + group[1] + group[0];
        }
        else if (size <= 256 * 1024){
            return _Index(size - 64 * 1024, 13) + group[3] + group[2] + group[1] + group[0];
        }
        else{
            assert(false);
        }
        return -1;
    }


    // tc向cc单次申请块空间的上限块数
    static size_t NumMoveSize(size_t size)
    {
        assert(size > 0);

        // MAX_BYTES就是单个块的最大空间，也就是256KB
        int num = MAX_BYTES / size; 

        if (num > 512)
        {
            /*比如说单次申请的是8B，256KB除以8B得到的是一个三万多的
            数，那这样单次上限三万多块太多了，直接开到三万多可能会造
            成很多浪费的空间，不太现实，所以该小一点*/
            num = 512;
        }

        // 如果说除了之后特别小，比2小，那么就调成2
        if (num < 2)
        {
            /*比如说单次申请的是256KB，那除得1，如果256KB上限一直是1
            ，那这样有点太少了，可能线程要的是4个256KB，那将num改成2
            就可以少调用几次，也就会少几次开销，但是也不能太多，256KB
            空间是很大的，num太高了不太现实，可能会出现浪费*/
            num = 2;
        }
        
        // [2, 512]，一次批量移动多少个对象的(慢启动)上限值
        // 小对象一次批量上限高
        // 小对象一次批量上限低

        return num;
    }

    // 块页匹配算法
    static size_t NumMovePage(size_t size)      // size表示一块的大小
    {   // 当cc中没有span为tc提供小块空间时，cc就需要向pc申请一块span，此时需要根据一块空间的大小来匹配
        // 出一个维护页空间较为合适的span，以保证span为size后尽量不浪费或不足够还再频繁申请相同大小的span

        // NumMoveSize是算出tc向cc申请size大小的块时的单次最大申请块数
        size_t num = NumMoveSize(size);
        
        // num * size就是单次申请最大空间大小
        size_t npage = num * size;

        /* PAGE_SHIFT表示一页要占用多少位，比如一页4KB就是12位，这里右
            移其实就是除以页大小，算出来就是单次申请最大空间有多少页*/
        npage >>= PAGE_SHIFT;

        /*如果算出来为0，那就直接给1页，比如说size为8B时，num就是512，npage
        算出来就是4KB，那如果一页8KB，算出来直接为0了，意思就是半页的空间都
        够8B的单次申请的最大空间了，但是二进制中没有0.5，所以只能给1页*/
        if (npage == 0)
            npage = 1;
        
        return npage;
    }
};