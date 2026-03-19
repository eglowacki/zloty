#include "Render/Pipeline/ConstantBuffer.h"
#include "Render/Platform/D3D12MemAlloc.h"


//--------------------------------------------------------------------------------------------------
yaget::render::ConstantBuffer::ConstantBuffer(D3D12MA::Allocation* allocation, ComPtr<ID3D12Resource> resource, constant_shader_types::RootType rootType, uint32_t index)
    : mAllocation(allocation)
    , mResource(resource)
    , mRootType(rootType)
    , mIndex(index)
{
}


//--------------------------------------------------------------------------------------------------
yaget::render::ConstantBuffer::~ConstantBuffer()
{
}


//--------------------------------------------------------------------------------------------------
bool yaget::render::ConstantBuffer::UpdateData(const void* data, size_t dataSize) const
{
    D3D12_RANGE emptyRange = {0, 0};
    void* mappedPtr;
    HRESULT hr = mResource->Map(0, &emptyRange, &mappedPtr);
    if (FAILED(hr))
    {
        YLOG_ERROR("REND", "Did not mapped resource for data updating.");
        return false;
    }

    memcpy(mappedPtr, data, dataSize);

    mResource->Unmap(0, nullptr);

    return true;
}

 

//--------------------------------------------------------------------------------------------------
void yaget::render::ConstantBuffer::Bind(ID3D12GraphicsCommandList* commandList) const
{
    //for (const auto& [key, value] : indexMap)
    //{
    //    if (value.mType == constant_shader_types::ConstantTypes::WorldViewProjection && value.mLayout == constant_shader_types::ConstantLayout::Matrix4x4)
    //    {
    //        const auto location = renderComponent->mMatrix;
    //        float matrix[16];
    //        math3d::GetMatrixAsFloats(location, matrix);
    //        std::ranges::fill(matrix, mMatrixInterpolator.GetValue(gameClock));

    //        if (value.mRootType == constant_shader_types::RootType::Constant)
    //        {
    //            commandList->SetGraphicsRoot32BitConstants(value.mOffset, 16, matrix, 0);
    //        }
    //        else if (value.mRootType == constant_shader_types::RootType::ConstantBufferView)
    //        {
    //            YAGET_ASSERT(false, "Not Implemented Yet!!!");
    //            //commandList->SetGraphicsRootConstantBufferView(value.mOffset, constantBufferView);
    //        }
    //    }
    //}


    D3D12_GPU_VIRTUAL_ADDRESS constBufGPUAddr = mResource->GetGPUVirtualAddress();

    commandList->SetGraphicsRootConstantBufferView(mIndex, constBufGPUAddr);
}
