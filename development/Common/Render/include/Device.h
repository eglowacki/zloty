/////////////////////////////////////////////////////////////////////////
// Device.h
//
//  Copyright 7/26/2016 Edgar Glowacki.
//
// NOTES:
//      Common functions and global utilities
//
//
// #include "Device.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "App/Application.h"
#include "Resources/ShaderResources.h"
#include "Resources/ResourceView.h"
#include "Resources/ConstantsResource.h"
#include "Resources/RenderStateCache.h"
#include "Resources/ResourceActivator.h"
#include "VTS/VirtualTransportSystem.h"
#include "Streams/Watcher.h"

namespace yaget
{
    class Application;
    namespace metrics { class Channel; }

    namespace render
    {
        namespace state { class DepthStencilStateResource; class BlendStateResource; class RasterizerStateResource; }
        class RenderTarget;
        class ShaderData;

        class Device
        {
        public:
            using ConvertorFunction = std::function<std::shared_ptr<render::ResourceView>(const std::shared_ptr<io::Asset>& asset, Device& device)>;
            using ResolverKey = std::pair<std::string, std::type_index>;
            using TagResourceResolvers = std::map<ResolverKey, Device::ConvertorFunction>;

            // handy typedef to switch between version 1 and 2 
            using ID3D11Device_t = ID3D11Device;
            using ID3D11DeviceContext_t = ID3D11DeviceContext;
            using IDXGISwapChain_t = IDXGISwapChain;

            struct Resolutions
            {
                struct Description
                {
                    uint32_t Width;
                    uint32_t Height;
                    uint32_t RefreshRate;
                };

                std::string AdapterName;
                std::string MonitorName;
                uint32_t Width = 0;
                uint32_t Height = 0;
                std::vector<Description> Modes;
            };

            // beginning of a Device class  
            Device & operator=(const Device&) = delete;
            Device(const Device&) = delete;
        
            Device(Application& app, const TagResourceResolvers& tagResolvers, io::Watcher& watcher);
            ~Device();

            // When major reset happens, (resolution, disconnect, etc), it will return new value
            // this can be used by other code to keep track of it needs to reset it's own stuff
            uint64_t GetSignature() const;

            // Should never cache these values, always ask device when needed
            ID3D11Device_t* GetDevice() const { return mHolder.mDevice.Get(); }
            ID3D11DeviceContext_t* GetDeviceContext() const 
            {
                YAGET_ASSERT(dev::CurrentThreadIds().IsThreadRender(), "Getting Device Context must be called from RENDER thread.");
                return mHolder.mDeviceContext.Get();
            }

            void Resize();
            void SurfaceStateChange();

            void SetConstants(const std::shared_ptr<ConstantsResource>& constants) { mConstants = constants; }
            void ActivateConstants() 
            {
                YAGET_ASSERT(mConstants, "Trying to activate NULL constants.");
                mConstants->Activate();
            }

            template <typename T>
            void UpdateConstant(const std::string& constantName, const std::string& slotName, const T& source)
            {
                YAGET_ASSERT(mConstants, "Trying to update NULL constants.");
                mConstants->Update(constantName, slotName, source);
            }

            using UserRenderCallback = std::function<void(const time::GameClock& gameClock, metrics::Channel& channel)>;
            void RenderFrame(const time::GameClock& gameClock, metrics::Channel& channel, render::RenderTarget* renderTarget, UserRenderCallback renderSceneCallback);
            Application& App() { return mApp; }
            const Application& App() const { return mApp; }

            static void DebugReport();

            RenderTarget* CreateRenderTarget(const std::string& name, uint32_t width, uint32_t height);
            RenderTarget* FindRenderTarget(const std::string& name) const;

            enum class ResourceLoadRequest { Deferred, Immediate };

            // management of rendering resources. Most based on vts and creates it's render view from it
            template<typename R, typename A>
            void RequestResourceView(const io::Tag& tag, std::function<void(std::shared_ptr<R>)> viewCallback, std::function<std::shared_ptr<R>(std::shared_ptr<A>)> convertCallback, uint64_t watchIt, ResourceLoadRequest loadRequest = ResourceLoadRequest::Deferred);
            template<typename R, typename A>
            void RequestResourceView(const std::vector<io::Tag>& tags, std::function<void(std::shared_ptr<R>)> viewCallback, std::function<std::shared_ptr<R>(std::shared_ptr<A>)> convertCallback, uint64_t watchIt, ResourceLoadRequest loadRequest = ResourceLoadRequest::Deferred);

            template<typename R, typename A>
            void RequestResourceView(const io::Tag& tag, mt::SmartVariable<R>& sink, uint64_t watchIt, ResourceLoadRequest loadRequest = ResourceLoadRequest::Deferred);
            template<typename R, typename A>
            void RequestResourceView(const std::vector<io::Tag>& tags, mt::SmartVariable<R>& sink, uint64_t watchIt, ResourceLoadRequest loadRequest = ResourceLoadRequest::Deferred);

            template<typename R>
            void RequestResourceView(const io::Tag& tag, mt::SmartVariable<R>& sink, uint64_t watchIt, ResourceLoadRequest loadRequest = ResourceLoadRequest::Deferred);

            template<typename R>
            std::shared_ptr<R> RequestResourceView(const io::Tag& tag);

            // Remove this watcher from the list
            void RemoveWatch(uint64_t watchIt);

            const io::VirtualTransportSystem::Sections& AvailableFonts() const { return mFonts; }

            std::vector<Resolutions> GetResolutions(Resolutions* currentResolution) const;
            Resolutions GetCurrentResolution() const;

            IDXGISwapChain_t* GetSwapChain() { return mHolder.mSwapChain.Get(); }

            // Exposes state cache to provide central management of render states, like which is active, resetting to previous setting, minimizing duplicate calls, etc.
            state::RenderStateCache& StateCache() { YAGET_ASSERT(mRenderStateCache, "StateCache is not created."); return *mRenderStateCache.get(); }

            // used in debugging to keep track of frame activation/rendering of resources
            void ActivatedResource(const ResourceView* resource, const char* userText = nullptr);

            state::ResourceActivator& ResourceActivator() { return mResourceActivator; }

        private:
            using ViewCallback = std::function<void(std::shared_ptr<ResourceView> /*resourceView*/)>;
            using ConvertCallback = std::function<std::shared_ptr<ResourceView>(std::shared_ptr<io::Asset> /*asset*/)>;
            void RequestResourceView(const std::vector<io::Tag>& tags, const std::type_info& resourceType, ViewCallback viewCallback, ConvertCallback convertCallback, uint64_t watchIt, ResourceLoadRequest loadRequest);
            void ResolveResourceRequest(const std::vector<io::Tag>& tags, const std::type_info& resourceType, ViewCallback viewCallback, ConvertCallback convertCallback, ResourceLoadRequest loadRequest);
            void Resize(std::function<void()> callback);

            ConvertorFunction GetConvertor(const io::Tag& tag, const std::type_info& resourceType) const;

            void Gui_UpdateWatcher();

            void ResetDevice();
            struct PrintMessage
            {
                math3d::Vector2 Pos;
                math3d::Color Color = math3d::Color(1, 1, 1, 1);
                std::string Message;
            };

            io::Watcher& mResourceWatcher;
            Application& mApp;
            static Microsoft::WRL::ComPtr<ID3D11Debug> DebugInterface;

            struct Holder
            {
                Microsoft::WRL::ComPtr<ID3D11Device_t> mDevice;
                Microsoft::WRL::ComPtr<ID3D11DeviceContext_t> mDeviceContext;
                Microsoft::WRL::ComPtr<IDXGISwapChain_t> mSwapChain;

                ~Holder();
            };
            Holder mHolder;

            PrintMessage mLastSettings;
            std::vector<PrintMessage> mPrintMessages;
            std::mutex mPrintMessagesMutex;

            struct Waiter
            {
                void Wait();

                void BeginPause();
                void EndPause();

                std::mutex mPauseRenderMutex;
                std::condition_variable mWaitForRenderThread;
                std::atomic_bool mPauseCounter{ false };
                std::condition_variable mRenderPaused;

                int mUsageCounter = 0;
            };

            struct WaiterScoper
            {
                WaiterScoper(Waiter& waiter) : waiter(waiter)
                {
                    waiter.BeginPause();
                }

                ~WaiterScoper()
                {
                    waiter.EndPause();
                }

                Waiter& waiter;
            };

            Waiter mWaiter;
            std::atomic_uint64_t mSignature{ 0 };

            // render target management
            typedef std::map<std::string, std::shared_ptr<RenderTarget>> RenderTargets_t;
            RenderTargets_t mRenderTargets;

            // resource view container
            // resource key are a combination of asset guid and resource typeid, since we allow creation of different
            // resources based on same asset. For example: shader code to create shader resource and constants resource
            using ResourceKey = std::pair<Guid, size_t>;
            using ViewResources = std::map<ResourceKey, std::shared_ptr<ResourceView>>;
            ViewResources mViewResources;
            std::mutex mResourcesMutex;

            // used by resource view to not block calling thread
            yaget::mt::JobPool mResourceRequests;
            io::VirtualTransportSystem::Sections mFonts;
            std::shared_ptr<ConstantsResource> mConstants;

            mutable std::vector<Resolutions> mResolutions;

            const TagResourceResolvers mTagResolvers;
            // caches render states to minimize activations
            std::unique_ptr<state::RenderStateCache> mRenderStateCache;

            struct DefaultState
            {
                Device* mDevice;

                mt::SmartVariable<render::state::DepthStencilStateResource> mDepth;
                mt::SmartVariable<render::state::RasterizerStateResource> mRasterizer;
                mt::SmartVariable<render::state::BlendStateResource> mBlend;
                std::size_t mDepthRefreshId = 0;
                std::size_t mRasterizerRefreshId = 0;
                std::size_t mBlendRefreshId = 0;

                using Section = io::VirtualTransportSystem::Section;
                void Request(const Section& depth, const Section& rasterizer, const Section& blend);

                bool GetBlocks(std::vector<state::ResourcePtr>& defaultStates) const;
                //void FreeResources(Device& device);

                ~DefaultState();
                void operator=(const DefaultState& source);
            };

            DefaultState mDefaultState;

            struct FrameResource 
            {
                const ResourceView* mResource;
                std::string mUserText;

                bool mHighlightType = false;
                bool mHighlightHash = false;
                bool mHighlightName = false;
            };

            using FrameResources = std::vector<FrameResource>;
            FrameResources mFrameResources;
            int mResourceBreakOn = -1;

            FrameResources mPreviousFrameResources;

            state::ResourceActivator mResourceActivator;

            app::SurfaceState mCurrentSurfaceState = app::SurfaceState::Shared;
        };

        //--------------------------------------------------------------------------------------------------------
        template<typename R, typename A>
        void Device::RequestResourceView(const io::Tag& tag, std::function<void(std::shared_ptr<R>)> viewCallback, std::function<std::shared_ptr<R>(std::shared_ptr<A>)> convertCallback, uint64_t watchIt, ResourceLoadRequest loadRequest /*= ResourceLoadRequest::Deferred*/)
        {
            RequestResourceView<R, A>(std::vector<io::Tag>{ tag }, viewCallback, convertCallback, watchIt, loadRequest);
        }

        template<typename R, typename A>
        void Device::RequestResourceView(const std::vector<io::Tag>& tags, std::function<void(std::shared_ptr<R>)> viewCallback, std::function<std::shared_ptr<R>(std::shared_ptr<A>)> convertCallback, uint64_t watchIt, ResourceLoadRequest loadRequest /*= ResourceLoadRequest::Deferred*/)
        {
            YAGET_ASSERT(viewCallback, "ViewCallback required for requested resource.");

            // wrap typed callback into ResourceView expected one, and then cast and call user typed callback
            auto wrappedViewCallback = [viewCallback](std::shared_ptr<ResourceView> resource)
            {
                std::shared_ptr<R> castResource = resource_cast<R>(resource);
                viewCallback(castResource);
            };

            auto wrappedConvertCallback = [this, convertCallback](std::shared_ptr<io::Asset> asset)
            {
                if (convertCallback)
                {
                    std::shared_ptr<A> userAsset = io::asset_cast<A>(asset);
                    std::shared_ptr<R> resource = convertCallback(userAsset);
                    return resource;
                }
                else
                {
                    auto convertor = Convertor<A, R>();

                    std::shared_ptr<A> userAsset = io::asset_cast<A>(asset);
                    std::shared_ptr<R> resource = convertor(userAsset, std::ref(*this));
                    return resource;
                }
            };

            RequestResourceView(tags, typeid(R), wrappedViewCallback, wrappedConvertCallback, watchIt, loadRequest);
        }

        //--------------------------------------------------------------------------------------------------------
        template<typename R, typename A>
        void Device::RequestResourceView(const io::Tag& tag, mt::SmartVariable<R>& sink, uint64_t watchIt, ResourceLoadRequest loadRequest /*= ResourceLoadRequest::Deferred*/)
        {
            RequestResourceView<R, A>(std::vector<io::Tag>{ tag }, sink, watchIt, loadRequest);
        }

        template<typename R, typename A>
        void Device::RequestResourceView(const std::vector<io::Tag>& tags, mt::SmartVariable<R>& sink, uint64_t watchIt, ResourceLoadRequest loadRequest /*= ResourceLoadRequest::Deferred*/)
        {
            // wrap typed callback into ResourceView expected one, and then cast and call user typed callback
            auto wrappedViewCallback = [&sink](std::shared_ptr<ResourceView> resource)
            {
                std::shared_ptr<R> castResource = resource_cast<R>(resource);
                sink = castResource;
            };

            auto wrappedConvertCallback = [this](std::shared_ptr<io::Asset> asset)
            {
                auto convertor = Convertor<A, R>();

                std::shared_ptr<A> userAsset = io::asset_cast<A>(asset);
                std::shared_ptr<R> resource = convertor(userAsset, std::ref(*this));
                return resource;
            };

            RequestResourceView(tags, typeid(R), wrappedViewCallback, wrappedConvertCallback, watchIt, loadRequest);
        }

        template<typename R>
        void Device::RequestResourceView(const io::Tag& tag, mt::SmartVariable<R>& sink, uint64_t watchIt, ResourceLoadRequest loadRequest /*= ResourceLoadRequest::Deferred*/)
        {
            // wrap typed callback into ResourceView expected one, and then cast and call user typed callback
            auto wrappedViewCallback = [&sink](std::shared_ptr<ResourceView> resource)
            {
                std::shared_ptr<R> castResource = resource_cast<R>(resource);
                sink = castResource;
            };

            auto wrappedConvertCallback = [this, tag](std::shared_ptr<io::Asset> asset)
            {
                auto convertor = GetConvertor(tag, typeid(R));
                YAGET_ASSERT(convertor, "There was no convertor from Tag for Asset: '%s' to type: '%s'.", (asset ? asset->mTag.ResolveVTS().c_str() : "NULL"), typeid(R).name());

                auto resource = convertor(asset, std::ref(*this));
                return resource;
            };

            RequestResourceView({ tag }, typeid(R), wrappedViewCallback, wrappedConvertCallback, watchIt, loadRequest);
        }

        template<typename R>
        std::shared_ptr<R> Device::RequestResourceView(const io::Tag& tag)
        {
            mt::SmartVariable<R> sink;
            RequestResourceView<R>(tag, sink, 0, ResourceLoadRequest::Immediate);

            std::shared_ptr<R> result = sink;

            return result;
        }

    } // namespace render
} // namespace yaget
