/////////////////////////////////////////////////////////////////////////
// RenderSceneItems.h
//
//  Copyright SwapChain.h Edgar Glowacki.
//
// NOTES:
//      
//
// #include "Render/Scene/RenderSceneItems.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"
#include "VTS/VirtualTransportSystem.h"
#include "Render/Pipeline/RenderGeometries.h"
#include "Render/Polygons/RenderShape.h"
#include "Render/Pipeline/RenderShaders.h"
#include "Render/Commands/RenderCommandTypes.h"

namespace yaget::app
{
    class WindowFrame;
}

struct ID3D12RootSignature;
struct ID3D12PipelineState;
struct ID3D12DescriptorHeap;

namespace yaget::render
{
    class ConstantBuffer;

    class RenderMaterialProperties;
    class RenderSignatures;
    class RenderPipelines;
    class ShaderBuffers;
    class TextureResources;
    class GeometriesResources;

    namespace commands
    {
        class CommandList;
    }

}


namespace yaget::render::scene
{
    class SceneItemsStorage;

    //-------------------------------------------------------------------------------------------------
    class SceneItem
    {
    public:
        static inline uint32_t PassOrderIndependent = 0;

        struct AssetTags
        {
            io::Tag mMaterialTag;
            io::Tags mGeometriesTags;
            io::Tags mTexturesTags;
            uint32_t mRenderPassOrder{ PassOrderIndependent };
            AssetCacheType mPsoCacheType{ AssetCacheType::Empty };
        };

        SceneItem();
        ~SceneItem();

        void Render(uint32_t bufferIndex, const commands::CommandList* commandList, commands::RenderPassState& currentRenderPassState);

        template <typename T>
        bool UpdateData(uint32_t bufferIndex, constant_shader_types::ConstantTypes constantTypes, const T& data, commands::Type commandType)
        {
            return UpdateData(bufferIndex, constantTypes, reinterpret_cast<const uint8_t*>(&data), sizeof(T), commandType);
        }

        // mRenderPassOrder is upper 32 bits and lower 32 bits
        // are combination of properties, like root, pipeline, constants, geometry data and texture resources.
        // That means the lower value of mRenderPassOrder will be rendered 'first'
        uint64_t GetRenderOrder() const;
        const AssetTags& GetTags() const;

    private:
        friend SceneItemsStorage;

        bool UpdateData(uint32_t bufferIndex, constant_shader_types::ConstantTypes constantTypes, const uint8_t* data, size_t dataSize, commands::Type commandType);

        ID3D12RootSignature* mRootSignature{};
        ID3D12PipelineState* mPipelineState{};
        ConstantBuffer* mConstantBuffer{};

        GeometriesResources::GeometryData mGeometriesData{};
        std::vector<ID3D12DescriptorHeap*> mTextureResources{};

        RenderShape mRenderShape;
        AssetTags mTags;
    };


    //-------------------------------------------------------------------------------------------------
    class SceneItemsStorage
    {
    public:
        using Section = io::VirtualTransportSystem::Section;
        using Sections = io::VirtualTransportSystem::Sections;

        SceneItemsStorage(RenderMaterialProperties& renderMaterials,
                          RenderSignatures& renderSignatures,
                          RenderPipelines& renderPipelines,
                          ShaderBuffers& shaderBuffers,
                          TextureResources& textureResources,
                          GeometriesResources& geometriesResources,
                          io::VirtualTransportSystem& vts, io::VirtualTransportSystem::Section fileName);

        ~SceneItemsStorage();

        SceneItem* GetSceneItem(const io::Tag& tag);
        std::vector<SceneItem*> GetSceneItems(const io::Tags& tags, comp::gs::mt::InitCounter* counter);

        static void SortSceneItems(std::vector<SceneItem*>& sceneItems);

        void Preload(const io::Tags& tags, comp::gs::mt::InitCounter& counter);
        void ResetAll(const app::WindowFrame& windowFrame);

        void ClearItem(const io::Tag& tag);

        static void PopulateMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);
        static void SaveMappings(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts);

    private:
        RenderMaterialProperties& mRenderMaterials;
        RenderSignatures& mSignatures;
        RenderPipelines& mPipelines;
        ShaderBuffers& mShaderBuffers;
        TextureResources& mTextures;
        GeometriesResources& mGeometries;
        io::VirtualTransportSystem& mVTS;

        using Items = std::map<io::Tag, SceneItem>;
        Items mItems;

        mutable std::shared_mutex mMutex;
    };
}
