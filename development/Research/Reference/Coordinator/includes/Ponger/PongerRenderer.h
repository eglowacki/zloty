//////////////////////////////////////////////////////////////////////
// PongerRenderer.h
//
//  Copyright 7/12/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "Ponger/PongerRenderer.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Components/Collectors.h"
#include "Components/LineComponent.h"
#include "Ponger/GameWorldSystem.h"
#include "Debugging/Primitives.h"
#include "Ponger/RenderWorldSystem.h"
#include "Streams/Buffers.h"
#include "ThreadModel/Variables.h"

namespace yaget::render
{
    class Device;
    class GeometryResource;
    class GeometryComponent;
    class TextComponent;
} // namespace yaget::render

namespace yaget::comp { class LocationComponent; }
namespace yaget::metrics { class Channel; }


namespace ponger
{
    using namespace yaget;

    using SceneChunkCollector = comp::CollectorHelper<comp::SceneChunk>;

    // This takes all active location components and adds that data to payload stager,
    // which in render callback will use that created payload (in thread safely manner) and will render it
    class PongerRenderer : public ponger::SceneChunkCollector
    {
    public:
        PongerRenderer(render::Device& device);
        ~PongerRenderer();

        void GatherLocation(yaget::comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, comp::LocationComponent* location, ponger::DebugComponent* debugComponenet);
        void Render(const yaget::time::GameClock& gameClock, metrics::Channel& channel);

        io::Tag mRectangleAssetTag;

    private:
        void ProcessPayload(typename const Stager::ConstPayload& payload);
        void InitializeGlobalComponents();
        void onProcessGui(comp::Component::UpdateGuiType updateGuiType);

        using RenderEntity = comp::RowPolicy<ponger::DebugDrawComponent*, render::GeometryComponent*>;
        using RenderEntityCoordinator = comp::Coordinator<RenderEntity>;

        using GlobalRenderEntity = comp::RowPolicy<render::LineComponent*, render::TextComponent*>;
        using GlobalRenderEntityCoordinator = comp::Coordinator<GlobalRenderEntity>;

        RenderEntityCoordinator mRenderEntityCoordinator;
        GlobalRenderEntityCoordinator mGlobalRenderEntityCoordinator;
        render::Device& mDevice;
        comp::ItemIds mIds;

        comp::Id_t mGlobalLineId = comp::INVALID_ID;

        ponger::LineCollectorSystem mLineCollectorSystem;
        ponger::LineRendererSystem mLineRendererSystem;
        ponger::RenderSystemCoordinator mRenderSystemCoordinator;

        mt::SmartVariable<render::GeometryResource> mGeometryResourceSink;

        struct ShaderPassInfo
        {
            std::string mName;
            bool mActive = false;
        };

        using ActivePasses = std::vector<ShaderPassInfo>;
        ActivePasses mActivePasses;
    };

    using RendererGatherSystem = yaget::comp::GameSystem<yaget::EndMarkerEntity, yaget::comp::LocationComponent*, ponger::DebugComponent*>;

} // namespace ponger