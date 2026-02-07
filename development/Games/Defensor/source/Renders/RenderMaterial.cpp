#include "Core/ErrorHandlers.h"
#include "Render/Platform/DeviceDebugger.h"
#include "Renders/RenderMaterial.h"
#include <CommonStates.h>
#include <d3dx12.h>
#include <Fmt/format.h>
#include <VertexTypes.h>

#include "Parsers/DependencyGraph.h"
#include "Streams/Buffers.h"


namespace
{
 

}

//-------------------------------------------------------------------------------------------------
defensor::render::RenderMaterial::RenderMaterial(Section materialSection, io::VirtualTransportSystem& vts)
    : mMaterialSection(materialSection)
    , mVTS(vts)
{
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderMaterial::~RenderMaterial() = default;
