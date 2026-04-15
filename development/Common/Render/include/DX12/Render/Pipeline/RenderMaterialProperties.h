///////////////////////////////////////////////////////////////////////
// RenderMaterialProperties.h
//
//  Copyright 02/14/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//  #include "Render/Pipeline/RenderMaterialProperties.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Render/Cache/CacheWatcher.h"

namespace yaget
{
    class DependencyGraph;
}


namespace yaget::render
{
    class PipelineTags;

    struct MaterialPropertyTags
    {
        io::Tag mVertexShader;
        io::Tag mPixelShader;
        io::Tag mSignature;
        io::Tag mPSO;
        io::Tag mShaderBuffer;
    };

    class RenderMaterialProperties
    {
    public:
        RenderMaterialProperties(PipelineTags& pipelineTags, io::VirtualTransportSystem& vts);
        ~RenderMaterialProperties();

        MaterialPropertyTags GetMaterial(const io::Tag& tag);
        std::vector<MaterialPropertyTags> GetMaterials(const io::Tags& tags);

        void ClearCache(const io::Tag& tag);

        static void PopulateMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
        static void SaveMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);

    private:
        PipelineTags& mPipelineTags;
        io::VirtualTransportSystem& mVTS;

        using MaterialTags = std::map<io::Tag, MaterialPropertyTags>;
        MaterialTags mMaterialTags;

        std::shared_mutex mMutexTags;
    };
}


template<>
struct yaget::conv::Convertor<yaget::render::MaterialPropertyTags>
{
    static std::string ToString(const yaget::render::MaterialPropertyTags& value)
    {
        return std::format("Material Properties:\n\tvs:          '{}'\n\tps:          '{}'\n\tSignature    '{}'\n\tPipeline     '{}'\n\tShaderBuffer '{}'.",
                           conv::ToString(value.mVertexShader),
                           conv::ToString(value.mPixelShader),
                           conv::ToString(value.mSignature),
                           conv::ToString(value.mPSO),
                           conv::ToString(value.mShaderBuffer));
    }
};
