/////////////////////////////////////////////////////////////////////////
// Scene.h
//
//  Copyright 7/27/2016 Edgar Glowacki.
//
// NOTES:
//  Scene collection and simple helper routines for interaction and management
//  with scene items
//
//
// #include "Scene.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#if 0

#include "MathFacade.h"
#include "Loaders/GeometryConvertor.h"
#include "Gui/VideoOptions.h"
#include "Debugging/Primitives.h"
#include <memory>
#include <vector>
#include <mutex>

namespace Concurrency
{
    namespace diagnostic
    {
        class marker_series;
    }
} // namespace Concurrency

namespace yaget
{
    class Application;
    namespace time { class GameClock; }
    namespace metrics { class Channel; }
    namespace io { class VirtualTransportSystem; }

    namespace render
    {
        class Device;

        //--------------------------------------------------------------------------------------------------
        class Scene
        {
        public:
            Scene& operator=(const Scene&) = delete;
            Scene(const Scene&) = delete;

            Scene(Device& device);
            ~Scene();

            void RunLogic(Application& app, const time::GameClock& gameClock, metrics::Channel& channel);
            void RunRender(Application& app, const time::GameClock& gameClock, metrics::Channel& channel);

            comp::ComponentPools& Pools() { return mComponentPools; }

            comp::Id_t NewItem(const math3d::Matrix& matrix);

            typedef std::shared_ptr<render::GeometryConvertor> Asset_t;

            comp::Id_t GetCameraId() const { return mCameraData.mCameraId; }
            comp::Id_t GetTerrainId() const { return mTerrainId; }

            struct TerrainMetadata
            {
                int Version = 0;
                double Latitude = 0.0;
                double Longitude = 0.0;
                math3d::Vector3 Origin;
            };

            const TerrainMetadata& GetTerrainMetadata() const { return mTerrainMetadata; }
            bool IsRenderTargetDepthMap() const { return mRenderTargetDepthMap; }
            void CameraAutoView();

        private:
            void PrepareData();
            void GuiPass();
            virtual void onGuiPass() {}
            void OnTerrainChunkLoaded(const comp::Component& from);

            Device& mDevice;

            // for now an entity, this needs to be defined somewhere
            struct Item
            {
                void Participate() {}

                std::shared_ptr<comp::LocationComponent> mLocationComp;
                std::shared_ptr<comp::PhysicsComponent> mPhysicsComp;
                std::shared_ptr<ModelComponent> mModelComp;
                std::shared_ptr<QuadComponent> mQuadComp;
                std::shared_ptr<GridComponent> mGridComp;
                std::shared_ptr<LineComponent> mLineComp;
                std::shared_ptr<CameraComponent> mCameraComp;
                std::shared_ptr<TerrainComponent> mTerrainComp;
            };

            typedef std::vector<Item> Items_t;

            typedef std::vector<comp::LocationComponentPool::TokenState> PrepareBuffers_t;

            struct PBHead
            {
                PrepareBuffers_t mBuffers;
                DirectX::SimpleMath::Matrix mProjectionMatrix;
                DirectX::SimpleMath::Matrix mViewMatrix;
                math::Box mSceneBoundingBox;    // used to center camera for depth buffer
            };
            typedef std::shared_ptr<PBHead> PBPtr_t;

            typedef RenderComponent::RenderBuffer RenderBuffer;
            typedef RenderComponent::RenderBuffers_t RenderBuffers_t;

            struct RBHead
            {
                RenderBuffers_t mBuffers;
                DirectX::SimpleMath::Matrix mProjectionMatrix;
                DirectX::SimpleMath::Matrix mViewMatrix;
                math::Box mSceneBoundingBox;    // used to center camera for depth buffer
            };
            typedef std::shared_ptr<RBHead> RBPtr_t;

            // Helper class to manage pointers of buffer data, moving them to a next state
            struct BufferStager
            {
                enum class Stager
                {
                    ES_PREPARE,
                    ES_RENDER
                };

                void SetStage(Stager id, PBPtr_t buffer);
                void SetStage(Stager id, RBPtr_t buffer);

                PBPtr_t GetAndClearStage(Stager id, PBPtr_t g);
                RBPtr_t GetAndClearStage(Stager id, RBPtr_t g);

                PBPtr_t GetNewStage(Stager id, PBPtr_t g) const;
                RBPtr_t GetNewStage(Stager id, RBPtr_t g) const;

                PBPtr_t GetStageValue(Stager id, PBPtr_t g) const;
                RBPtr_t GetStageValue(Stager id, RBPtr_t g) const;

                PBPtr_t mPrepareReady;
                RBPtr_t mRenderReady;
            };

            BufferStager mLogicToRenderStager;

            static PBPtr_t PBR_SET;
            static RBPtr_t RBR_SET;

            struct CameraData
            {
                float mViewportRatio = 1.65f;
                float mFOV = 60.0f;
                float mNear = 1.0f;
                float mFar = 10000.0f;
                comp::Id_t mCameraId = 0;
                bool mAutoView = false;     // keep object in view at all times
            };
            CameraData mCameraData;

            // items/component management
            comp::ComponentPools mComponentPools;
            Items_t mItems;

            struct BulletDebug                
            {
                bool mWireFrame = true;
                bool mAabb = false;
                float mGravity = -10.0f;
            };

            BulletDebug mBulletDebug;

            bool mRenderTargetDepthMap = true;	// controls what to render to texture, depth or color
            int mDepthMapSize = 3;				// dimensions of depth map render buffer

            typedef std::map<std::string, std::shared_ptr<render::GeometryConvertor>> Assets_t;
            Assets_t mAssets;
            mutable std::mutex mmAssetsMutex;

            comp::Id_t mTerrainId = 0;
            TerrainMetadata mTerrainMetadata;
            enum class SavingStage
            {
                None,
                Gray,
                Color
            };
            SavingStage mSavingState = SavingStage::None;

            yaget::gui::VideoOptions mVideoOptions;
        };

    } // namespace render
} // namespace yaget
#endif // 0