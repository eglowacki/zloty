#include "Core/ErrorHandlers.h"

#include "Render/Pipeline/ShaderBuffers.h"
#include "Render/Helpers/ResourceDescriptions.h"
#include "Render/Pipeline/ConstantBuffer.h"
#include "Render/Platform/Adapter.h"
#include "Render/Platform/D3D12MemAlloc.h"


//--------------------------------------------------------------------------------------------------
yaget::render::ShaderBuffers::ShaderBuffers(int numBuffers, const platform::Adapter& adapter, io::VirtualTransportSystem& /*vts*/, io::VirtualTransportSystem::Section /*fileName*/)
    : mAdapter(adapter)
{
    for (int i = 0; i < numBuffers; ++i)
    {
        mConstantResources[i] = {};
    }
}


//--------------------------------------------------------------------------------------------------
yaget::render::ShaderBuffers::~ShaderBuffers()
{
    mt::WriteLock locker(mMutex);

    std::ranges::for_each(mConstantResources, [](auto& element)
    {
        std::ranges::for_each(element.second, [](auto& entry)
        {
            entry.mResource.Reset();
            entry.mAllocation->Release();
        });
    });
}


//--------------------------------------------------------------------------------------------------
void yaget::render::ShaderBuffers::MakeBuffers(const io::Tag& tag, const RenderShaders::IndexMap& indexMap)
{
    mt::WriteLock locker(mMutex);

    ConstantBuffer::ShaderVariables shaderVariables;

    for (const auto& value : indexMap | std::views::values)
    {
        if (value.mRootType == constant_shader_types::RootType::Constant)
        {
            ConstantBuffer::ShaderVariable shaderVariable(nullptr, nullptr, value.mRootType, value.mType, value.mLayout, value.mOffset, this);
            shaderVariables.push_back(std::move(shaderVariable));
        }
        else if (value.mRootType == constant_shader_types::RootType::ConstantBufferView)
        {
            size_t dataWidth = 0;
            if (value.mType == constant_shader_types::ConstantTypes::WorldViewProjection && value.mLayout == constant_shader_types::ConstantLayout::Matrix4x4)
            {
                dataWidth = sizeof(float) * 16;
            }
            else if (value.mType == constant_shader_types::ConstantTypes::Time)
            {
                if (value.mLayout == constant_shader_types::ConstantLayout::Float)
                {
                    dataWidth = sizeof(float);
                }
                else if (value.mLayout == constant_shader_types::ConstantLayout::Float4)
                {
                    dataWidth = sizeof(float) * 4;
                }
            }
            else
            {
                YAGET_ASSERT(false, std::format("Unsupported DataWidth for constant buffer, Type: '{}', Layout: '{}'", magic_enum::enum_name(value.mType), magic_enum::enum_name(value.mLayout)).c_str());
            }

            AddConstantResource(tag, dataWidth);

            ConstantBuffer::ShaderVariable shaderVariable(nullptr, nullptr, value.mRootType, value.mType, value.mLayout, value.mOffset, this);
            shaderVariables.push_back(std::move(shaderVariable));
        }
        else if (value.mRootType == constant_shader_types::RootType::Table)
        {
            if (value.mType == constant_shader_types::ConstantTypes::Texture2d)
            {
                ConstantBuffer::ShaderVariable shaderVariable(nullptr, nullptr, value.mRootType, value.mType, value.mLayout, value.mOffset, this);
                shaderVariables.push_back(std::move(shaderVariable));
            }
            else if (value.mType == constant_shader_types::ConstantTypes::Sampler)
            {
                ConstantBuffer::ShaderVariable shaderVariable(nullptr, nullptr, value.mRootType, value.mType, value.mLayout, value.mOffset, this);
                shaderVariables.push_back(std::move(shaderVariable));
            }
            else
            {
                YAGET_ASSERT(false, std::format("Unsupported root type: '{}' for type: '{}'.", magic_enum::enum_name(value.mRootType), magic_enum::enum_name(value.mType)).c_str());
            }
        }
        else
        {
            YAGET_ASSERT(false, std::format("Unsupported root type: '{}'.", magic_enum::enum_name(value.mRootType)).c_str());
        }
    }

    mBuffersMap[tag] = std::make_shared<ConstantBuffer>(shaderVariables);
}


//--------------------------------------------------------------------------------------------------
yaget::render::ConstantBuffer* yaget::render::ShaderBuffers::GetBuffer(const io::Tag& tag)
{
    mt::ReadLock locker(mMutex);

    if (auto it = mBuffersMap.find(tag); it != mBuffersMap.end())
    {
        return it->second.get();
    }

    YLOG_ERROR("REND", std::format("Could not find constant buffer for tag: '{}'", conv::ToString(tag)).c_str());
    return {};
}


//--------------------------------------------------------------------------------------------------
yaget::render::ComPtr<ID3D12Resource> yaget::render::ShaderBuffers::GetNextResource(uint32_t bufferIndex, size_t dataSize)
{
    mt::WriteLock locker(mMutex);

    if (mCurrentIndexBuffer != bufferIndex)
    {
        mCurrentIndexBuffer = bufferIndex;
        std::ranges::for_each(mConstantResources[mCurrentIndexBuffer], [](auto& element)
        {
            element.mUsed = false;
        });
    }

    auto resource = FindNextFreeResource(bufferIndex, dataSize);
    if (!resource)
    {
        YLOG_WARNING("REND", "There is no available Constant Resource for index: '%d' with size: '%d', adding more...", bufferIndex, dataSize);
        AddConstantResource({}, dataSize);
        resource = FindNextFreeResource(bufferIndex, dataSize);
    }

    YAGET_ASSERT(resource, "Could not get available Constant Resource for index: '%d' with size: '%d'.", bufferIndex, dataSize);
    return resource;
}


//--------------------------------------------------------------------------------------------------
yaget::render::ComPtr<ID3D12Resource> yaget::render::ShaderBuffers::FindNextFreeResource(uint32_t bufferIndex, size_t dataSize)
{
    auto it = std::ranges::find_if(mConstantResources[bufferIndex], [dataSize](auto& element)
    {
        if (!element.mUsed && element.mSize == dataSize)
        {
            element.mUsed = true;
            return true;
        }

        return false;
    });

    if (it != mConstantResources[bufferIndex].end())
    {
        return it->mResource;
    }

    return {};
}


//--------------------------------------------------------------------------------------------------
void yaget::render::ShaderBuffers::AddConstantResource(const io::Tag& tag, size_t size)
{
    size_t numConstantResources = 10;

    mNumberOfResources[size]++;;

    size_t numResources = 0;
    std::ranges::for_each(mConstantResources, [&numResources, size](const auto& element)
    {
        numResources += std::ranges::count_if(element.second, [size](const auto& item)
        {
            return item.mSize == size;
        });
    });

    if (mNumberOfResources[size] * numConstantResources > numConstantResources)
    {
        numResources = 0;
    }

    if (numResources == 0)
    {
        auto allocator = mAdapter.GetAllocator();

        std::ranges::for_each(mConstantResources, [tag, size, allocator, numConstantResources](auto& element)
        {
            auto& resources = element.second;

            for (size_t i = 0; i < numConstantResources; ++i)
            {
                D3D12MA::Allocation* allocation = helpers::CreateUploadHeap(tag, size, allocator);
                ComPtr<ID3D12Resource> resource = allocation->GetResource();

                resources.push_back({ .mAllocation = allocation, .mResource = resource, .mSize = size, .mUsed = false });
            }
        });
    }
}
