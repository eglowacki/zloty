/////////////////////////////////////////////////////////////////////////
// RenderComponent.h
//
//  Copyright 7/26/2016 Edgar Glowacki.
//
// NOTES:
//      Base class for any render type of components
//
//
// #include "Components/RenderComponent.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/Component.h"
#include "MathFacade.h"
#include "Platform/Support.h"
#include "ThreadModel/JobPool.h"
#include "ThreadModel/FileLoader.h"
#include "ThreadModel/Variables.h"
#include "Loaders/GeometryConvertor.h"
#include "Streams/GeometryStream.h"
#include "App/AppUtilities.h"
#include "Debugging/DevConfiguration.h"
#include "StringCRC.h"
#include "Fmt/format.h"
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <fstream>

namespace yaget
{
    //namespace comp
    //{
    //    // Attribute/property types we support
    //    typedef struct Unused {} Unused;    // dummy placeholder
    //    typedef struct Screen {} Screen;    // use screen space to render
    //    typedef struct Color {} Color;      // encode color

    //    template <typename T, typename R = T>
    //    struct Attribute;

    //    template<> struct Attribute<Unused, Unused>;

    //    template<> struct Attribute<Screen>
    //    {
    //        typedef bool Type;
    //        bool& Value() { return mSpace; }
    //        //const Type& Value() const { return mSpace; }

    //    private:
    //        bool mSpace = false;
    //    };

    //    template<> struct Attribute<Color>
    //    {
    //        typedef DirectX::SimpleMath::Color Type;
    //        Type& Value() { return mColor; }
    //        //const Type& Value() const { return mColor; }

    //    private:
    //        DirectX::SimpleMath::Color mColor = DirectX::SimpleMath::Color(1,1,1,1);
    //    };

    //    //class AttributeCollection
    //    //{
    //    //public:
    //    //private:
    //    //    std::
    //    //};


    //} // namespace comp

    namespace render
    {
        class Device;
        class Scene;
        class ShaderMaterial;
        class RenderTarget;
        
        //--------------------------------------------------------------------------------------------------
        class RenderComponent : public comp::Component
        {
        public:
            static const uint32_t SignalReset = "RenderComponent.SignalReset"_crc32;

            virtual ~RenderComponent();

            // Once the prepare thread gets PrepareBuffer, it will walk over and generate device dependent layout of data
            // and that in turn will be fed to render::RenderComponent
            struct RenderBuffer
            {
                comp::Id_t Id = comp::INVALID_ID;
                DirectX::SimpleMath::Matrix Matrix;
            };
            using RenderBuffers_t = std::vector<RenderBuffer>;
            using RBPtr_t = std::shared_ptr<RenderBuffers_t>;

            struct RenderOptions
            {
                bool bShowGUI = false;
            };

            void Render(const RenderTarget* renderTarget, const math3d::Matrix& matrix);
            void Render(const RenderBuffer& renderBuffer, const DirectX::SimpleMath::Matrix& viewMatrix, const DirectX::SimpleMath::Matrix& projectionMatrix, const RenderComponent::RenderOptions* renderOptions);
            void Reset();
            // controls visibility/rendering of this component (thread safe)
            void AttachAsset(const io::Tag& tag) { AttachAsset(std::vector<io::Tag>{ tag }); }
            void AttachAsset(const std::vector<io::Tag>& tags);

        protected:
            enum class Init { Default, AutoReset };
            RenderComponent(comp::Id_t id, Device& device, Init initOptions, const io::Tags& tags);

            Device& mDevice;
            mt::Variable<io::Tags> mTags;

        private:
            virtual void OnReset() = 0;
            virtual void onRender(const RenderTarget* /*renderTarget*/, const math3d::Matrix& /*matrix*/) {}
            virtual void OnRender(const RenderBuffer& renderBuffer, const DirectX::SimpleMath::Matrix& viewMatrix, const DirectX::SimpleMath::Matrix& projectionMatrix) = 0;
            virtual void OnGuiRender(const RenderBuffer& renderBuffer, const DirectX::SimpleMath::Matrix& viewMatrix, const DirectX::SimpleMath::Matrix& projectionMatrix) = 0;

            // used to control rendering of this component
            std::atomic_bool mValid;
            std::atomic_bool mResetActivated;
            std::atomic_bool mChangedTags;
            uint64_t mDeviceSignature = 0;
        };

    } // namespace render
} // namespace yaget
