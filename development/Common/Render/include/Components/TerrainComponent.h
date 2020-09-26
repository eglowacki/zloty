/////////////////////////////////////////////////////////////////////////
// TerrainComponent.h
//
//  Copyright 3/30/2017 Edgar Glowacki.
//
// NOTES:
//      Provides terrain rendering
//
//
// #include "Components/TerrainComponent.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#ifndef YAGET_RENDER_TERRAIN_COMPONENT_H
#define YAGET_RENDER_TERRAIN_COMPONENT_H
#pragma once

#include "Math/MathBase.h"
#include "Components/RenderComponent.h"
#include "Resources/ShaderResources.h"
#include "ThreadModel/Variables.h"
#include "VTS/VirtualTransportSystem.h"



namespace yaget
{
    namespace render
    {
        class TerrainComponent : public RenderComponent
        {
        public:
            using Section = io::VirtualTransportSystem::Section;
            using Sections = io::VirtualTransportSystem::Sections;

            static const uint32_t SignalChunkLoaded = "TerrainComponent.SignalChunkLoaded"_crc32;

            TerrainComponent(comp::Id_t id, Device& device, Scene& scene);

            void Tick(const time::GameClock& gameClock) override;

            void ImportTerrain(const Section& section) { ImportTerrain(Sections{ section }); }
            void ImportTerrain(const Sections& sections);

            math::Box BoundingBox() const;

        private:                                                                                                                                
            size_t CalculateStateHash() const override { YAGET_ASSERT(false, "Implemented CalculateStateHash methos!!!"); return 0; }

            void OnReset() override;
            void OnRender(const RenderBuffer& renderBuffer, const DirectX::SimpleMath::Matrix& viewMatrix, const DirectX::SimpleMath::Matrix& projectionMatrix) override;
            void OnGuiRender(const RenderBuffer& renderBuffer, const DirectX::SimpleMath::Matrix& viewMatrix, const DirectX::SimpleMath::Matrix& projectionMatrix) override;

            void OnChunkUpdate(const comp::Component& from);

            std::vector<comp::Id_t> mChunkIds;
            mutable mt::Variable<math::Box> mTerrainBounds;
            double mChunkStreamTime = 0.0;
            Scene& mScene;
        };


        class TerrainComponentPool : public RenderComponentPool<TerrainComponent, 50>
        {
        public:
            TerrainComponentPool()
                : RenderComponentPool<TerrainComponent, 50>()
            {}

            Ptr New(comp::Id_t id, Device& device, Scene& scene);
        };

    } // namespace render
} // namespace yaget

#endif // YAGET_RENDER_TERRAIN_COMPONENT_H
