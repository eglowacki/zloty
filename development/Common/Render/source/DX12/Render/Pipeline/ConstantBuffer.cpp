#include "Render/Pipeline/ConstantBuffer.h"
#include "Render/Platform/D3D12MemAlloc.h"
#include "Render/Platform/DeviceDebugger.h"


//--------------------------------------------------------------------------------------------------
yaget::render::ConstantBuffer::ConstantBuffer(const ShaderVariables& shaderVariables)
    : mShaderVariables(shaderVariables)
{
}


//--------------------------------------------------------------------------------------------------
yaget::render::ConstantBuffer::~ConstantBuffer()
{
    std::vector<D3D12MA::Allocation*> allocationsToRelease = std::ranges::views::transform(mShaderVariables,
                                                                                           [](const auto& shaderVariable) { return shaderVariable.mAllocation; }) |
        std::views::filter([](const auto& shaderVariable) { return shaderVariable != nullptr; }) |
        std::ranges::to<std::vector>();

    mShaderVariables.clear();
    std::ranges::for_each(allocationsToRelease, [](D3D12MA::Allocation* allocation) { allocation->Release(); });
}


//--------------------------------------------------------------------------------------------------
bool yaget::render::ConstantBuffer::UpdateData(constant_shader_types::ConstantTypes constantTypes, const uint8_t* data, size_t dataSize)
{
    if (auto variable = FindVariable(constantTypes))
    {
        if (variable->mRootType == constant_shader_types::RootType::Constant)
        {
            mVariableUpdateData[constantTypes] = io::CreateBuffer(data, dataSize);
        }
        else if (variable->mRootType == constant_shader_types::RootType::ConstantBufferView)
        {
            D3D12_RANGE emptyRange = { 0, 0 };
            void* mappedPtr = nullptr;
            HRESULT hr = variable->mResource->Map(0, &emptyRange, &mappedPtr);
            if (FAILED(hr))
            {
                auto debugName = YAGET_RENDER_GET_DEBUG_NAME(variable->mResource.Get());
                YLOG_ERROR("REND", "Did not mapped resource '%s' for updating data.", debugName.c_str());

                return false;
            }

            memcpy(mappedPtr, data, dataSize);

            variable->mResource->Unmap(0, nullptr);
        }
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
bool yaget::render::ConstantBuffer::UpdateData(constant_shader_types::ConstantTypes constantTypes, const void* data, size_t dataSize)
{
    return UpdateData(constantTypes, static_cast<const uint8_t*>(data), dataSize);
}


//--------------------------------------------------------------------------------------------------
void yaget::render::ConstantBuffer::Bind(ID3D12GraphicsCommandList* commandList) const
{
    for (const auto& variable : mShaderVariables)
    {
        if (variable.mRootType == constant_shader_types::RootType::Constant)
        {
            if (auto it = mVariableUpdateData.find(variable.mConstantType); it != mVariableUpdateData.end())
            {
                const auto& dataBuffer = it->second;
                commandList->SetGraphicsRoot32BitConstants(variable.mIndex, static_cast<uint32_t>(io::size_data(dataBuffer) / sizeof(float)), io::cast_data<const float>(dataBuffer), 0);
            }
        }
        else if (variable.mRootType == constant_shader_types::RootType::ConstantBufferView)
        {
            D3D12_GPU_VIRTUAL_ADDRESS constBufGPUAddr = variable.mResource->GetGPUVirtualAddress();
            commandList->SetGraphicsRootConstantBufferView(variable.mIndex, constBufGPUAddr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
const yaget::render::ConstantBuffer::ShaderVariable* yaget::render::ConstantBuffer::FindVariable(constant_shader_types::ConstantTypes constantType) const
{
    auto it = std::ranges::find_if(mShaderVariables, [constantType](const auto& element)
    {
        return element.mConstantType == constantType;
    });

    if (it != mShaderVariables.end())
    {
        return &(*it);
    }

    return nullptr;
}
