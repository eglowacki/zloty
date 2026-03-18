#include "Render/Pipeline/ShaderBuffers.h"
#include "Render/Platform/Adapter.h"
#include "Render/Platform/D3D12MemAlloc.h"


//--------------------------------------------------------------------------------------------------
yaget::render::ConstantBuffers::ConstantBuffers(const platform::Adapter& adapter, io::VirtualTransportSystem& /*vts*/, io::VirtualTransportSystem::Section /*fileName*/)
    : mAdapter(adapter)
{
    
}


//--------------------------------------------------------------------------------------------------
yaget::render::ConstantBuffers::~ConstantBuffers()
{
    
}


//--------------------------------------------------------------------------------------------------
void yaget::render::ConstantBuffers::MakeBuffers(const io::Tag& /*tag*/, const RenderShaders::IndexMap& /*indexMap*/)
{
}


//--------------------------------------------------------------------------------------------------
yaget::render::ConstantData yaget::render::ConstantBuffers::GetBuffer(const io::Tag& /*tag*/)
{
    return ConstantData{};
}
