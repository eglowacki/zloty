#include "App/AppUtilities.h"
#include "Core/ErrorHandlers.h"
#include "Render/Platform/Adapter.h"
#include "Render/Platform/D3D12MemAlloc.h"

#include <d3dx12.h>

namespace
{
#if YAGET_DEBUG_RENDER == 1
    struct MemoryEntry
    {
        size_t mSize{};
        size_t mOffset{};
    };
    using MemoryTracker = std::unordered_map<void*, MemoryEntry>;

    MemoryTracker memoryTracker;

    void PrintMemoryTracker()
    {
        YLOG_INFO("GMEM", "=== Graphics Memory Report ===");
        for (auto [mem, entry] : memoryTracker)
        {
            YLOG_INFO("GMEM", "Left Memory: '%p ->%llu'", mem, entry.mSize);
        }
    }
#else
    void PrintMemoryTracker() {}
#endif


    //-------------------------------------------------------------------------------------------------
    void* GraphicsAllocate(size_t size, size_t alignment, void* /*pPrivateData*/)
    {
        void* memory = _aligned_malloc(size, alignment);

#if YAGET_DEBUG_RENDER == 1
        //YLOG_INFO("GMEM", "Allocate Size: '%llu -> %p'", size, memory);
        memoryTracker[memory] = { size, alignment };
#endif

        return memory;
    }


    //-------------------------------------------------------------------------------------------------
    void GraphicsFree(void* memory, void* /*pPrivateData*/)
    {
        if (memory)
        {
#if YAGET_DEBUG_RENDER == 1
            //YLOG_INFO("GMEM", "Free: '%p'", memory);
            auto& element = memoryTracker[memory];
            element.mSize -= _aligned_msize(memory, element.mOffset, 0);
            if (memoryTracker[memory].mSize == 0)
            {
                memoryTracker.erase(memory);
            }
#endif

            _aligned_free(memory);
        }
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::render::platform::PrintD3D12MAMemoryTracker()
{
    PrintMemoryTracker();
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::Adapter::Adapter([[maybe_unused]] app::WindowFrame windowFrame, const info::Adapter& adapterInfo)
    : mAllocationCallbacks(std::make_unique<D3D12MA::ALLOCATION_CALLBACKS>(&GraphicsAllocate, &GraphicsFree, nullptr))
{
    auto [device, adapter, factory] = info::CreateDevice(adapterInfo);
    HRESULT hr = device.As(&mDevice);
    error_handlers::ThrowOnError(hr, "Could not get Device from CreateDevice.");

    hr = adapter.As(&mAdapter);
    error_handlers::ThrowOnError(hr, "Could not get Adapter from CreateDevice.");

    hr = factory.As(&mFactory);
    error_handlers::ThrowOnError(hr, "Could not get Factory from CreateDevice.");

#if YAGET_DEBUG_RENDER == 1
    mDeviceDebugger.ActivateMessageSeverity(mDevice);
#endif // YAGET_DEBUG_RENDER == 1

    D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
    allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;
    allocatorDesc.pDevice = mDevice.Get();
    allocatorDesc.PreferredBlockSize = 0;
    allocatorDesc.pAllocationCallbacks = mAllocationCallbacks.get();
    allocatorDesc.pAdapter = mAdapter.Get();

    D3D12MA::Allocator* allocator = nullptr;
    hr = D3D12MA::CreateAllocator(&allocatorDesc, &allocator);
    error_handlers::ThrowOnError(hr, "Could not get D3D12MA Allocator");
    mAllocator.reset(allocator);
}


//-------------------------------------------------------------------------------------------------
yaget::render::platform::Adapter::~Adapter() = default;


//-------------------------------------------------------------------------------------------------
ID3D12Device* yaget::render::platform::Adapter::GetDevice() const
{
    return mDevice.Get();
}


//-------------------------------------------------------------------------------------------------
IDXGIFactory* yaget::render::platform::Adapter::GetFactory() const
{
    return mFactory.Get();
}


//-------------------------------------------------------------------------------------------------
D3D12MA::Allocator* yaget::render::platform::Adapter::GetAllocator() const
{
    return mAllocator.get();
}

