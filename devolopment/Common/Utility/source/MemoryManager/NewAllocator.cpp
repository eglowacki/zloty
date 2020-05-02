#include "YagetCore.h"


#if YAGET_NEW_ALLOCATOR_ENABLED == 1
#pragma message("======== Yaget New Allocator Included ========")

void* operator new  (std::size_t count)
{
    void* ptr = std::malloc(count);
    if (ptr)
    {
        return ptr;
    }
    else
    {
        throw std::bad_alloc{};
    }
}

void* operator new[](std::size_t count)
{

}

void* operator new  (std::size_t count, std::align_val_t al)
{

}

void* operator new[](std::size_t count, std::align_val_t al)
{

}
#else
#pragma message("======== Yaget New Allocator NOT Included ========")

DISREGARD_LINKER_4221(COMPILER_VERIFICATION_NewAllocator_h)

#endif // YAGET_NEW_ALLOCATOR_ENABLED
