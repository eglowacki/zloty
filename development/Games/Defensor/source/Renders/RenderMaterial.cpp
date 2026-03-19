#include "Renders/RenderMaterial.h"
#include "Streams/Buffers.h"
#include "VTS/ResolvedAssets.h"

//-------------------------------------------------------------------------------------------------
defensor::render::RenderMaterial::RenderMaterial(const io::Tag& assetTag)
    : mAssetTag(assetTag)
{
}


//-------------------------------------------------------------------------------------------------
defensor::render::RenderMaterial::~RenderMaterial() = default;
