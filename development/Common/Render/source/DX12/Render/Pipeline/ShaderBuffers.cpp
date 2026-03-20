#include "Core/ErrorHandlers.h"

#include "Render/Pipeline/ShaderBuffers.h"
#include "Render/Pipeline/ConstantBuffer.h"
#include "Render/Platform/Adapter.h"
#include "Render/Platform/D3D12MemAlloc.h"


//--------------------------------------------------------------------------------------------------
yaget::render::ShaderBuffers::ShaderBuffers(const platform::Adapter& adapter, io::VirtualTransportSystem& /*vts*/, io::VirtualTransportSystem::Section /*fileName*/)
    : mAdapter(adapter)
{
}


//--------------------------------------------------------------------------------------------------
yaget::render::ShaderBuffers::~ShaderBuffers() = default;


//--------------------------------------------------------------------------------------------------
void yaget::render::ShaderBuffers::MakeBuffers(const io::Tag& tag, const RenderShaders::IndexMap& indexMap)
{
    ConstantBuffer::ShaderVariables shaderVariables;
    //std::shared_ptr<ConstantBuffer> constantBuffer;
    auto allocator = mAdapter.GetAllocator();

    for (const auto& value : indexMap | std::views::values)
    {
        if (value.mRootType == constant_shader_types::RootType::Constant)
        {
            ConstantBuffer::ShaderVariable shaderVariable(nullptr, nullptr, value.mRootType, value.mType, value.mLayout, value.mOffset);
            shaderVariables.push_back(std::move(shaderVariable));
        }
        else if (value.mRootType == constant_shader_types::RootType::ConstantBufferView)
        {
            D3D12MA::ALLOCATION_DESC allocationDesc = {};
            allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;   // D3D12MA::ALLOCATION_FLAG_WITHIN_BUDGET;
            allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;       // ALLOCATION_FLAG_WITHIN_BUDGET

            size_t dataWidth = 0;
            if (value.mType == constant_shader_types::ConstantTypes::WorldViewProjection && value.mLayout == constant_shader_types::ConstantLayout::Matrix4x4)
            {
                dataWidth = sizeof(float) * 16;
            }
            else if (value.mType == constant_shader_types::ConstantTypes::Time && value.mLayout == constant_shader_types::ConstantLayout::Float4)
            {
                dataWidth = sizeof(float) * 4;
            }
            else
            {
                YAGET_ASSERT(false, std::format("Unsupported DataWidth for constant buffer, Type: '{}', Layout: '{}'", magic_enum::enum_name(value.mType), magic_enum::enum_name(value.mLayout)).c_str());
            }

            D3D12_RESOURCE_DESC1 resourceDesc = {};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Alignment = 0;
            resourceDesc.Width = dataWidth;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.SampleDesc.Quality = 0;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            D3D12MA::Allocation* allocation = nullptr;
            ComPtr<ID3D12Resource> resource;
            HRESULT hr = allocator->CreateResource2(
                &allocationDesc,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                &allocation,
                IID_PPV_ARGS(&resource));

            error_handlers::ThrowOnError(hr, std::format("Could not allocate constant buffer for tag: {}", conv::ToString(tag)));

#if YAGET_DEBUG
            std::string debugName = std::format("ConstantBuffer_{}", value.mVariableName);
            allocation->SetName(conv::utf8_to_wide(debugName).c_str());
            YAGET_RENDER_SET_DEBUG_NAME(resource.Get(), debugName);
#endif

            ConstantBuffer::ShaderVariable shaderVariable(allocation, resource, value.mRootType, value.mType, value.mLayout, value.mOffset);
            shaderVariables.push_back(std::move(shaderVariable));
        }
        else
        {
            YAGET_ASSERT(std::format("Unsupported root type for constant buffer: '{}'", magic_enum::enum_name(value.mRootType)).c_str());
        }
    }

    mBuffersMap[tag] = std::make_shared<ConstantBuffer>(shaderVariables);
}


//--------------------------------------------------------------------------------------------------
yaget::render::ConstantBuffer* yaget::render::ShaderBuffers::GetBuffer(const io::Tag& tag)
{
    if (auto it = mBuffersMap.find(tag); it != mBuffersMap.end())
    {
        return it->second.get();
    }

    YLOG_ERROR("REND", std::format("Could not find constant buffer for tag: '{}'", conv::ToString(tag)).c_str());
    return {};
}
