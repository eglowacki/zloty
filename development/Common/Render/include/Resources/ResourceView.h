//////////////////////////////////////////////////////////////////////
// ResourceView.h
//
//  Copyright 7/12/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      notes_missing
//
//
//  #include "Resources/ResourceView.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "ThreadModel/Variables.h"
#include "Components/Component.h"
#include "Streams/Buffers.h"
#include <type_traits>
#include <typeindex>


namespace yaget
{
    namespace render
    {
        class Device;

        class ResourceView : public Noncopyable<ResourceView>
        {
        public:
            ResourceView(Device& device, const io::Tag& tag, const std::type_index& typeIndex) : mAssetTag(tag), mDevice(device), mTypeIndex(typeIndex)
            {}
            virtual ~ResourceView() 
            {}

            virtual const char* GetNameType() const = 0;
            virtual bool Activate() = 0;
            virtual void UpdateGui(comp::Component::UpdateGuiType /*updateGuiType*/) {}

            std::size_t GetStateHash() const;
            const std::type_index& GetType() const { return mTypeIndex; }
            const io::Tag mAssetTag;

            template<typename T>
            T* GetPlatformResource() const { return reinterpret_cast<T*>(mPlatformResource); }

        protected:
            void SetHashValue(std::size_t hashValue) { mHashValue = hashValue; }
            void SetPlatformResource(void* platformResource) { mPlatformResource = platformResource; }

            Device& mDevice;

        private:
            std::type_index mTypeIndex;
            std::size_t mHashValue = 0;
            void* mPlatformResource = nullptr;
        };

        //--------------------------------------------------------------------------------------------------
        //! Helper function to cast one type of asset smart pointer into another (A = T), it uses dynamic cast in debug, otherwise static
        template <typename A, typename T>
        inline std::shared_ptr<A> resource_cast(std::shared_ptr<T> resource)
        {
            static_assert(std::is_base_of_v<T, A>, "Cant not convert type T into type A");

#ifdef YAGET_DEBUG
            std::shared_ptr<A> castResource = std::dynamic_pointer_cast<A>(resource);
            YAGET_ASSERT(castResource, "Could not cast Resource from: '%s' to: '%s'.", typeid(T).name(), typeid(A).name());
#else
            std::shared_ptr<A> castResource = std::static_pointer_cast<A>(resource);
#endif // YAGET_DEBUG

            return castResource;
        }

        //--------------------------------------------------------------------------------------------------
        template <typename A, typename R>
        struct Convertor
        {
            std::shared_ptr<R> operator()(std::shared_ptr<A> asset, render::Device& device) const
            {
                return std::make_shared<R>(device, asset);
            }
        };


        //--------------------------------------------------------------------------------------------------
        template <typename R>
        struct Reloader
        {
            void operator()(std::shared_ptr<R> resource, mt::SmartVariable<R>& sink) const
            {
                YAGET_ASSERT(sink.empty(), "While waiting for resource, sink is not null.");
                sink = resource;
            }
        };

        namespace gui
        {
            void UpdateSectionText(const std::string& nodeText, const colors::Color& textColor, render::ResourceView* resourceView, comp::Component::UpdateGuiType updateGuiType);


        } // namespace gui
    } // namespace render
} // namespace yaget
#pragma once
