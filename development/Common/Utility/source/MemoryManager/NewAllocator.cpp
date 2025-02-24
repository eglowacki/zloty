#include "YagetCore.h"
#include "MemoryManager\NewAllocator.h"


#if defined(YAGET_NEW_ALLOCATOR_ENABLED) && (YAGET_NEW_ALLOCATOR_ENABLED == 1)
YAGET_COMPILE_GLOBAL_SETTINGS("Allocator Included")

#include <cstdio>
#include <cstdlib>
#include <source_location>
#include <stacktrace>

#if 0

namespace
{
    // copied from https://learn.microsoft.com/en-us/cpp/standard-library/allocators?view=msvc-170
    template <class T>
    struct Mallocator
    {
        typedef T value_type;
        Mallocator() noexcept {} //default ctor not required by C++ Standard Library

        // A converting copy constructor:
        template<class U> Mallocator(const Mallocator<U>&) noexcept {}
        template<class U> bool operator==(const Mallocator<U>&) const noexcept
        {
            return true;
        }
        template<class U> bool operator!=(const Mallocator<U>&) const noexcept
        {
            return false;
        }
        T* allocate(const size_t n) const;
        void deallocate(T* const p, size_t) const noexcept;
    };

    template <class T>
    T* Mallocator<T>::allocate(const size_t n) const
    {
        if (n == 0)
        {
            return nullptr;
        }
        if (n > static_cast<size_t>(-1) / sizeof(T))
        {
            throw std::bad_array_new_length();
        }
        void* const pv = malloc(n * sizeof(T));
        if (!pv) { throw std::bad_alloc(); }
        return static_cast<T*>(pv);
    }

    template<class T>
    void Mallocator<T>::deallocate(T* const p, size_t) const noexcept
    {
        free(p);
    }

    // this allows us to use stacktrace in new and delete functions, without triggering stack overflow.
    // The stack overflow will accure with default std::stacktrace because it allocates memory while
    // calling std::stacktrace::current() method.
    using Stacktrace = std::basic_stacktrace<Mallocator<std::stacktrace_entry>>;

    using MapAllocator = Mallocator<std::pair<const size_t, Stacktrace>>;
    using MemoryMap = std::map<size_t, Stacktrace, std::less<size_t>, MapAllocator>;
    MemoryMap& GetMemoryMap()
    {
        static MemoryMap memoryMap;
        return memoryMap;
    }

    std::mutex& GetMemoryMapMutex()
    {
        static std::mutex memoryMapMutex;
        return memoryMapMutex;
    }

    std::atomic_int& MemoryRecordingCounter()
    {
        static std::atomic_int memoryRecorder = 0;

        return memoryRecorder;
    }

    void AddMemoryAllocation(Stacktrace currentStacktrace, void* ptr)
    {
        size_t address = reinterpret_cast<size_t>(ptr);

        auto& memoryMap = GetMemoryMap();

        std::unique_lock<std::mutex> locker(GetMemoryMapMutex());
        memoryMap.insert(std::make_pair(address, currentStacktrace));
    }

    void RemoveMemoryAllocation(void* ptr)
    {
        size_t address = reinterpret_cast<size_t>(ptr);

        auto& memoryMap = GetMemoryMap();

        std::unique_lock<std::mutex> locker(GetMemoryMapMutex());
        memoryMap.erase(address);
    }

    void ReportMemoryAllocations()
    {
        std::unique_lock<std::mutex> locker(GetMemoryMapMutex());

        auto& memoryMap = GetMemoryMap();
        memoryMap;

        int z = 0;
        z;
    }

} // namespace


void* operator new(std::size_t count)
{
    void* ptr = std::malloc(count);
    if (ptr)
    {
        if (MemoryRecordingCounter())
        {
            const auto currentStacktrace = Stacktrace::current();
            AddMemoryAllocation(currentStacktrace, ptr);
        }

        return ptr;
    }
    else
    {
        throw std::bad_alloc{};
    }
}

void operator delete(void* ptr)
{
    if (MemoryRecordingCounter())
    {
        RemoveMemoryAllocation(ptr);
    }

    std::free(ptr);
}

void* operator new[](std::size_t count)
{
    void* ptr = std::malloc(count);
    if (ptr)
    {
        if (MemoryRecordingCounter())
        {
            const auto currentStacktrace = Stacktrace::current();
            AddMemoryAllocation(currentStacktrace, ptr);
        }

        return ptr;
    }
    else
    {
        throw std::bad_alloc{};
    }
}

void operator delete[](void* ptr)
{
    if (MemoryRecordingCounter())
    {
        RemoveMemoryAllocation(ptr);
    }

    std::free(ptr);
}

void* operator new(std::size_t count, std::align_val_t al)
{
    void* ptr = _aligned_malloc(count, size_t(al));
    if (ptr)
    {
        if (MemoryRecordingCounter())
        {
            const auto currentStacktrace = Stacktrace::current();
            AddMemoryAllocation(currentStacktrace, ptr);
        }

        return ptr;
    }
    else
    {
        throw std::bad_alloc{};
    }
}

void operator delete(void* ptr, [[maybe_unused]] std::align_val_t al)
{
    if (MemoryRecordingCounter())
    {
        RemoveMemoryAllocation(ptr);
    }

    _aligned_free(ptr);
}

void* operator new[](std::size_t count, std::align_val_t al)
{
    void* ptr = _aligned_malloc(count, size_t(al));
    if (ptr)
    {
        if (MemoryRecordingCounter())
        {
            const auto currentStacktrace = Stacktrace::current();
            AddMemoryAllocation(currentStacktrace, ptr);
        }

        return ptr;
    }
    else
    {
        throw std::bad_alloc{};
    }
}

void operator delete[](void* ptr, [[maybe_unused]] std::align_val_t al)
{
    if (MemoryRecordingCounter())
    {
        RemoveMemoryAllocation(ptr);
    }

    _aligned_free(ptr);
}

void yaget::memory::InitializeAllocations()
{

}

void yaget::memory::StartRecordAllocations()
{
    MemoryRecordingCounter()++;
}

void yaget::memory::StopRecordAllocations()
{
    MemoryRecordingCounter()--;
}

void yaget::memory::ReportAllocations()
{
    ReportMemoryAllocations();
}
#else //0

void yaget::memory::StartRecordAllocations() {}
void yaget::memory::StopRecordAllocations() {}
void yaget::memory::ReportAllocations() {}

#endif //0

#else
YAGET_COMPILE_GLOBAL_SETTINGS("Allocator NOT Included")

void yaget::memory::StartRecordAllocations() {}
void yaget::memory::StopRecordAllocations() {}
void yaget::memory::ReportAllocations() {}

#endif // YAGET_NEW_ALLOCATOR_ENABLED

int MyAllocHook(int allocType, void* userData, std::size_t size, int blockType, long requestNumber, const unsigned char* filename, int lineNumber)
{
    allocType;
    userData;
    size;
    blockType;
    requestNumber;
    filename;
    lineNumber;

    if (allocType == _HOOK_ALLOC)
    {
        int z = 0;
        z;
    }
    else if (allocType == _HOOK_REALLOC)
    {
        int z = 0;
        z;
    }
    else if (allocType == _HOOK_FREE)
    {
        int z = 0;
        z;
    }

    return true;
}


void yaget::memory::InitializeAllocations()
{
    _CrtSetAllocHook(MyAllocHook);
}
