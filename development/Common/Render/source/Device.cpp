#include "Platform/WindowsLean.h"
#include "Device.h"
#include "Resources/ShaderResources.h"
#include "Resources/DescriptionResource.h"
#include "Resources/DepthStencilStateResource.h"
#include "Resources/RasterizerStateResource.h"
#include "Resources/BlendStateResource.h"
#include "VTS/RenderConfigAssets.h"
#include "Gui/Support.h"
#include "RenderTarget.h"
#include "Resources/RenderStateCache.h"

using namespace yaget;
using namespace Microsoft::WRL;
using namespace DirectX;
namespace fs = std::filesystem;

namespace
{
    std::string makeShaderKey(const std::string& vertexName, const std::string& pixelName)
    {
        return vertexName + "." + pixelName;
    }

    // return IDXGIFactory used in managing full screen and window mode
    ComPtr<IDXGIFactory> GetDXGIFactory(render::Device::ID3D11Device_t* hardwareDevice)
    {
        ComPtr<IDXGIDevice> dxgDevice;
        HRESULT hr = hardwareDevice->QueryInterface(__uuidof(IDXGIDevice), &dxgDevice);
        YAGET_THROW_ON_RROR(hr, "Could not get IDXG IDevice.");

        ComPtr<IDXGIAdapter> dxgAdapter;
        hr = dxgDevice->GetAdapter(&dxgAdapter);
        YAGET_THROW_ON_RROR(hr, "Could not get IDXG Adapter.");

        ComPtr<IDXGIFactory> dxgFactory;
        hr = dxgAdapter->GetParent(__uuidof(IDXGIFactory), &dxgFactory);
        YAGET_THROW_ON_RROR(hr, "Could not get IDXG Factor from IDXG Adapter.");
        return dxgFactory;
    }

    std::vector<IDXGIAdapter*> EnumerateAdapters(IDXGIFactory* dxgFactory)
    {
        IDXGIAdapter* pAdapter;
        std::vector <IDXGIAdapter*> vAdapters;

        for (UINT i = 0; dxgFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            vAdapters.push_back(pAdapter);
        }

        return vAdapters;
    }

    std::vector<yaget::render::Device::Resolutions> GetValidResolutions(IDXGIFactory* dxgFactory, HMONITOR currentMonitor, uint32_t minimumRefreshRate)
    {
        using namespace yaget;
        const uint32_t kMinimumRefreshRate = minimumRefreshRate;

        std::vector<IDXGIAdapter*> adapters = EnumerateAdapters(dxgFactory);
        std::vector<render::Device::Resolutions> supportedModes;

        for (auto it : adapters)
        {
            DXGI_ADAPTER_DESC adapterDesc = {};
            HRESULT hr = it->GetDesc(&adapterDesc);

            UINT i = 0;
            IDXGIOutput* dxgOutput;
            std::vector<IDXGIOutput*> vOutputs;
            while (it->EnumOutputs(i, &dxgOutput) != DXGI_ERROR_NOT_FOUND)
            {
                vOutputs.push_back(dxgOutput);
                ++i;
            }

            for (auto out : vOutputs)
            {
                DXGI_OUTPUT_DESC outpuDesc = {};
                hr = out->GetDesc(&outpuDesc);

                render::Device::Resolutions resolutions;
                resolutions.AdapterName = conv::wide_to_utf8(adapterDesc.Description);
                resolutions.MonitorName = conv::wide_to_utf8(outpuDesc.DeviceName);
                resolutions.Width = outpuDesc.DesktopCoordinates.right - outpuDesc.DesktopCoordinates.left;
                resolutions.Height = outpuDesc.DesktopCoordinates.bottom - outpuDesc.DesktopCoordinates.top;

                if (currentMonitor && currentMonitor == outpuDesc.Monitor)
                {
                    return { resolutions };
                }

                UINT numModes = 0;
                DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

                // Get the number of elements
                hr = out->GetDisplayModeList(format, 0, &numModes, nullptr);

                std::vector<DXGI_MODE_DESC> modes(numModes);;

                // Get the list
                hr = out->GetDisplayModeList(format, 0, &numModes, modes.data());

                uint32_t width = resolutions.Width;
                uint32_t height = resolutions.Height;
                modes.erase(std::remove_if(std::begin(modes), std::end(modes),
                    [width, height, kMinimumRefreshRate](const DXGI_MODE_DESC& modeDesc)
                    {
                        uint32_t refreshRate = static_cast<uint32_t>(std::ceil(modeDesc.RefreshRate.Numerator / (modeDesc.RefreshRate.Denominator * 1.0f)));
                        if (refreshRate < kMinimumRefreshRate)
                        {
                            return true;
                        }
                        if (modeDesc.Scaling != DXGI_MODE_SCALING_UNSPECIFIED)
                        {
                            return true;
                        }

                        //if ((width % modeDesc.Width) == 0 && (height % modeDesc.Height) == 0)
                        //{
                        //    return false;
                        //}

                        return false;
                    }
                ), std::end(modes));

                for (auto mode : modes)
                {
                    uint32_t refreshRate = static_cast<uint32_t>(std::ceil(mode.RefreshRate.Numerator / (mode.RefreshRate.Denominator * 1.0f)));
                    resolutions.Modes.push_back({ mode.Width, mode.Height, refreshRate });
                }

                supportedModes.push_back(resolutions);
            }
        }

        return supportedModes;
    }


} // namespace

ComPtr<ID3D11Debug> render::Device::DebugInterface;


//--------------------------------------------------------------------------------------------------
std::vector<yaget::render::Device::Resolutions> yaget::render::Device::GetResolutions(Resolutions* currentResolution) const
{ 
    ComPtr<IDXGIFactory> dxgFactory = GetDXGIFactory(mHolder.mDevice.Get());
    if (currentResolution)
    {
        *currentResolution = GetCurrentResolution();
    }

    mResolutions = GetValidResolutions(dxgFactory.Get(), nullptr, 0);
    return mResolutions; 
}

//--------------------------------------------------------------------------------------------------
yaget::render::Device::Resolutions yaget::render::Device::GetCurrentResolution() const
{
    HWND hWnd = App().GetSurface().Handle<HWND>();
    HMONITOR windowMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

    ComPtr<IDXGIFactory> dxgFactory = GetDXGIFactory(mHolder.mDevice.Get());
    std::vector<Resolutions> resolutions = GetValidResolutions(dxgFactory.Get(), windowMonitor, InvalidId);
    YAGET_ASSERT(!resolutions.empty(), "There is no valid desktop/monitor/resolution conbination.");
    Resolutions currentResolution = resolutions.empty() ? Resolutions() : *resolutions.begin();

    DEVMODEA deviceInfo = {};
    deviceInfo.dmSize = sizeof(DEVMODEA);
    bool result = ::EnumDisplaySettingsA(nullptr, ENUM_CURRENT_SETTINGS, &deviceInfo);
    YAGET_UTIL_THROW_ON_RROR(result, "EnumDisplaySettingsA could not get device info.");

    uint32_t frequency = deviceInfo.dmDisplayFrequency;
    const auto& winSize = App().GetSurface().Size();
    //GetWindowSize();
    currentResolution.Modes.push_back({ static_cast<uint32_t>(winSize.x), static_cast<uint32_t>(winSize.y), frequency });
    return currentResolution;
}

//--------------------------------------------------------------------------------------------------
render::Device::Device(Application& app, const TagResourceResolvers& tagResolvers, io::Watcher& watcher)
    : mResourceWatcher(watcher)
    , mApp(app)
    , mResourceRequests("vts.RenderResource")
    , mTagResolvers(tagResolvers)
    , mDefaultState{ this }
{
    using Section = io::VirtualTransportSystem::Section;
    const Section deviceConfig{ dev::CurrentConfiguration().mGraphics.mDevice };

    using Config = io::SingleBLobLoader<io::JsonAsset>;
    Config blobLoader(App().VTS(), deviceConfig);
    auto deviceAsset = blobLoader.GetAsset<io::render::DeviceAsset, Config::DefaultAsset>();
    YLOG_CWARNING("DEVV", deviceAsset->mTag.IsValid(), "Device Config asset '%s' is not valid, using default values: {%s}.", deviceConfig.ToString().c_str(), deviceAsset->ToString().c_str());

    HWND hWnd = App().GetSurface().Handle<HWND>();

    const D3D_FEATURE_LEVEL* featureaLevel = deviceAsset->mFeatureLevels.data();
    const uint32_t numFeatures = static_cast<uint32_t>(deviceAsset->mFeatureLevels.size());

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> deviceContext;
    ComPtr<IDXGISwapChain1> swapChain;

    D3D_FEATURE_LEVEL selectedFeatureLevel;
    uint32_t deviceFlags = 0;
#ifdef YAGET_DEBUG
    deviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif // YAGET_DEBUG

    HRESULT hr = D3D11CreateDevice(nullptr,
                                   deviceAsset->mType,
                                   nullptr,
                                   deviceFlags,
                                   featureaLevel,
                                   numFeatures,
                                   D3D11_SDK_VERSION,
                                   &device,
                                   &selectedFeatureLevel,
                                   &deviceContext);

    YAGET_THROW_ON_RROR(hr, "Could not initialize DX11 render device");

    ComPtr<IDXGIDevice2> device2;
    hr = device->QueryInterface(__uuidof(IDXGIDevice2), &device2);
    YAGET_THROW_ON_RROR(hr, "Device QueryInterface does not support DX11.2");

    ComPtr<IDXGIAdapter> adapter;
    hr = device2->GetParent(__uuidof(IDXGIAdapter), &adapter);
    YAGET_THROW_ON_RROR(hr, "Could not initialize IDXGIAdapter from render device2");

    ComPtr<IDXGIFactory2> factory;
    hr = adapter->GetParent(__uuidof(IDXGIFactory2), &factory);
    YAGET_THROW_ON_RROR(hr, "Could not initialize IDXGIFactory2 from adapter");

    if (hWnd)
    {
        // Articles about how to create multisampling render target and copy that into swap chain,
        // since FLIP model does not allow for multisampling surface
        // https://docs.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_effect
        // 
        // Flip model article
        // https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/for-best-performance--use-dxgi-flip-model
        DXGI_SWAP_CHAIN_DESC1 swapDescription = {};
        swapDescription.Format = deviceAsset->mBufferFormat;
        swapDescription.SampleDesc.Count = deviceAsset->mMultiSample;
        if (swapDescription.SampleDesc.Count > 1)
        {
            swapDescription.SampleDesc.Quality = static_cast<UINT>(D3D11_STANDARD_MULTISAMPLE_PATTERN);
        }
        swapDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapDescription.BufferCount = deviceAsset->mBufferCount;
        swapDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapDescription.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        hr = factory->CreateSwapChainForHwnd(device2.Get(),
                                             hWnd,
                                             &swapDescription,
                                             nullptr,
                                             nullptr,
                                             &swapChain);

        YAGET_THROW_ON_RROR(hr, "Could not create swap chain");
    }

    // DX11.2
    D3D11_FEATURE_DATA_D3D11_OPTIONS1 dx2caps;
    hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS1, &dx2caps, sizeof(dx2caps));
    YAGET_THROW_ON_RROR(hr, "Device does not support DX11.2");

    hr = device.As(&mHolder.mDevice);
    YAGET_THROW_ON_RROR(hr, "Device does not support DX11.2 interface");
    YAGET_SET_DEBUG_NAME(mHolder.mDevice.Get(), "Device");

    if (hWnd)
    {
        ComPtr<IDXGIFactory> dxgFactory = GetDXGIFactory(device.Get());
        dxgFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
    }

#ifdef YAGET_DEBUG
    hr = mHolder.mDevice.As(&Device::DebugInterface);
    YAGET_THROW_ON_RROR(hr, "Did not get debug DX11.2 interface");
#endif // YAGET_DEBUG

    D3D11_FEATURE_DATA_THREADING thredData = {};
    hr = mHolder.mDevice->CheckFeatureSupport(D3D11_FEATURE_THREADING, &thredData, sizeof(D3D11_FEATURE_DATA_THREADING));
    YAGET_THROW_ON_RROR(hr, "Check Feature Support failed");
    YLOG_INFO("DEVV", "Current driver %s multithreading.", thredData.DriverConcurrentCreates ? "supports" : "does not support");

    hr = deviceContext.As(&mHolder.mDeviceContext);
    YAGET_THROW_ON_RROR(hr, "Context does not support DX11.2 interface");
    YAGET_SET_DEBUG_NAME(mHolder.mDeviceContext.Get(), "DeviceContext");

//YAGET_COMPILE_SUPPRESS_START(4189, "'': local variable is initialized but not referenced")
//
//    DXGI_FORMAT requestedFormats[] = {
//        DXGI_FORMAT_R8G8B8A8_UNORM,
//        DXGI_FORMAT_R8G8B8A8_UINT,
//        DXGI_FORMAT_R16G16B16A16_UNORM,
//        DXGI_FORMAT_R16G16B16A16_FLOAT,
//        DXGI_FORMAT_R32G32B32A32_FLOAT,
//        DXGI_FORMAT_R32G32B32A32_UINT
//    };
//
//    for (auto f : requestedFormats)
//    {
//        UINT formatSupport = 0;
//        hr = mHolder.mDevice->CheckFormatSupport(f, &formatSupport);
//
//        int a_renderTargetSupported = (D3D11_FORMAT_SUPPORT_RENDER_TARGET & formatSupport);
//        int a_bufferSupported = (D3D11_FORMAT_SUPPORT_BUFFER & formatSupport);
//
//        int a_renderMSTargetSupported = (D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET & formatSupport);
//        int a_displaySupported = (D3D11_FORMAT_SUPPORT_DISPLAY & formatSupport);
//        int a_MSbufferResolveSupported = (D3D11_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE & formatSupport);
//        int a_depthSupported = (D3D11_FORMAT_SUPPORT_DEPTH_STENCIL & formatSupport);
//        int z = 0;
//    }
//YAGET_COMPILE_SUPPRESS_END

    if (swapChain)
    {
        hr = swapChain.As(&mHolder.mSwapChain);
        YAGET_THROW_ON_RROR(hr, "SwapChain does not support DX11.2 interface");
    }

    // info only -BEGIN ==========================================================================================
    ComPtr<IDXGIDevice> dxgiDevice;
    hr = mHolder.mDevice.As(&dxgiDevice);
    YAGET_THROW_ON_RROR(hr, "Did not get DXGI device");

    DXGI_ADAPTER_DESC desc;
    hr = adapter->GetDesc(&desc);
    YAGET_THROW_ON_RROR(hr, "Adapter could not get description");

    D3D_FEATURE_LEVEL featureLevel = mHolder.mDevice->GetFeatureLevel();

    YLOG_INFO("DEVV", "Current adapter: '%s' with future levels: '%d'", conv::wide_to_utf8(desc.Description).c_str(), featureLevel);
    // info only - END  ==========================================================================================

    ResetDevice();
    yaget::gui::Initialize(*this);
}

render::Device::~Device()
{
    yaget::gui::Shutdown();

    if (mHolder.mSwapChain && mCurrentSurfaceState == app::SurfaceState::Exclusive)
    {
        HRESULT hr = mHolder.mSwapChain->SetFullscreenState(false, nullptr);
        YAGET_THROW_ON_RROR(hr, "Could not SetFullscreenState to desktop.");
    }
}

void render::Device::RenderFrame(const time::GameClock& gameClock, metrics::Channel& channel, render::RenderTarget* renderTarget, UserRenderCallback renderSceneCallback)
{
    mPreviousFrameResources = mFrameResources;
    mFrameResources = {};

    render::RenderTarget* backBuffer = FindRenderTarget("BackBuffer");

    renderTarget = renderTarget ? renderTarget : backBuffer;
    YAGET_ASSERT(renderTarget, "No render target exist when tring to render the frame.");

    if (dev::CurrentConfiguration().mDebug.mFlags.Gui && backBuffer == renderTarget)
    {
        renderTarget->AddProcessor(render::RenderTarget::ProcessorType::PostFrame, [this]() { Gui_UpdateWatcher(); });
    }

    std::vector<state::ResourcePtr> defaultStates;
    if (mDefaultState.GetBlocks(defaultStates))
    {
        ActivatedResource(nullptr, "    === Render Frame Started ===");

        if (mRenderStateCache->BeginFrame(defaultStates))
        {
            RenderTarget::Activator activator(*renderTarget);
            if (renderSceneCallback)
            {
                renderSceneCallback(gameClock, channel);
            }

            mRenderStateCache->EndFrame();
        }
    }

    if (renderTarget->IsScreenshotToTake())
    {
        renderTarget->SaveToFile();
    }

    mWaiter.Wait();
}

void render::Device::Waiter::Wait()
{
    if (mPauseCounter == true)
    {
        YLOG_NOTICE("DEVI", "Waiter - We are requested to pause. Stopping.");
        mWaitForRenderThread.notify_one();
        std::unique_lock<std::mutex> locker(mPauseRenderMutex);
        mRenderPaused.wait(locker);
        YLOG_NOTICE("DEVI", "Waiter - Resuming Render.");
    }
}

void render::Device::Waiter::BeginPause()
{
    // TODO Look at re-entrent lock (from the same thread)
    // rather then home grown
    if (mUsageCounter++)
    {
        return;
    }

    // We should use Concurency (perf) locker to keep track in RAD
    YLOG_NOTICE("DEVI", "Waiter - Requesting Render pause...");
    std::unique_lock<std::mutex> locker(mPauseRenderMutex);
    mPauseCounter = true;
    mWaitForRenderThread.wait(locker);
    YLOG_NOTICE("DEVI", "Waiter - Render is Paused (resizing commences...)");
}

void render::Device::Waiter::EndPause()
{
    if (--mUsageCounter)
    {
        return;
    }

    YLOG_NOTICE("DEVI", "Waiter - Render can start (resizing done)");
    mPauseCounter = false;
    mRenderPaused.notify_one();
}

void render::Device::SurfaceStateChange()
{
    // we need to pause render thread, and then
    if (mHolder.mSwapChain)
    {
        WaiterScoper scoper(mWaiter);

        Resize([this]()
        {
            if (mCurrentSurfaceState == app::SurfaceState::Shared)
            {
                YLOG_NOTICE("DEVI", "Switching to Full Screen...");

                std::vector<Device::Resolutions> resolutions = GetResolutions(nullptr);


                auto sorter = [](const auto& elem1, const auto& elem2)
                {
                    if (elem1.Width < elem2.Width)
                    {
                        return true;
                    }
                    else if (elem1.Width == elem2.Width)
                    {
                        return elem1.Height < elem2.Height;
                    }

                    return false;
                };

                for (auto& resMonitor : resolutions)
                {
                    //resMonitor;
                    std::sort(resMonitor.Modes.begin(), resMonitor.Modes.end(), sorter);
                    auto it = std::unique(resMonitor.Modes.begin(), resMonitor.Modes.end(), [](const auto& elem1, const auto& elem2)
                    {
                        return elem1.Width == elem2.Width && elem1.Height == elem2.Height;
                    });
                    resMonitor.Modes.resize(std::distance(resMonitor.Modes.begin(), it));
                }

                DXGI_MODE_DESC modeDesc{ 3840, 2160 };

                mCurrentSurfaceState = app::SurfaceState::Exclusive;
                HRESULT hr = mHolder.mSwapChain->ResizeTarget(&modeDesc);
                YAGET_THROW_ON_RROR(hr, "Could not ResizeTarget");

                hr = mHolder.mSwapChain->SetFullscreenState(true, nullptr);
                YAGET_THROW_ON_RROR(hr, "Could not SetFullscreenState to Fullscreen");
            }
            else if (mCurrentSurfaceState == app::SurfaceState::Exclusive)
            {
                YLOG_NOTICE("DEVI", "Switching to Shared...");

                mCurrentSurfaceState = app::SurfaceState::Shared;

                HRESULT hr = mHolder.mSwapChain->SetFullscreenState(false, nullptr);
                YAGET_THROW_ON_RROR(hr, "Could not SetFullscreenState to desktop");
            }
        });
    }
}

void render::Device::Resize(std::function<void()> callback)
{
    WaiterScoper scoper(mWaiter);
    try
    {
        if (callback)
        {
            callback();
        }

        ResetDevice();
        // only change signature if we successful.
        // TODO! what else can we do in case of failure here?
        mSignature = idspace::get_burnable(App().IdCache);
    }
    catch (const ex::standard& e)
    {
        YLOG_ERROR("DEVV", "ResetDevice of render device failed. %s", e.what());
        std::string message = fmt::format("ResetDevice of render device failed.\nExamine log at: '{}'\n{}", util::ExpendEnv("$(LogFolder)", nullptr), e.what());
        std::string errorTitle = fmt::format("{} Device Reset Error", util::ExpendEnv("$(AppName)", nullptr));
        util::DisplayDialog(errorTitle.c_str(), message.c_str());
    }
}

void render::Device::Resize()
{
    Resize(nullptr);
}

void render::Device::ResetDevice()
{
    // NOTE: how are we going to feed which fonts are available
    mPreviousFrameResources = {};
    mFrameResources = {};
    mRenderStateCache = {};
    mDefaultState = DefaultState{ this };
    mConstants = nullptr;
    mFonts = { { "FontFaces@Consola" }, { "FontFaces@Title" }, { "FontFaces@Large" }, { "FontFaces@Medium" }, { "FontFaces@Small" } };
    yaget::gui::Reseter guiReseter(*this);

    {
        std::unique_lock<std::mutex> locker(mResourcesMutex);
        mViewResources.clear();
    }

    const auto& surface = App().GetSurface(); 
    SimpleMath::Vector2 winSize = surface.Size();
    YLOG_NOTICE("DEVI", "Resetting Device with Surface resolution: (%dx%d).", static_cast<int>(winSize.x), static_cast<int>(winSize.y));

    ID3D11RenderTargetView* nullViews[] = { nullptr };
    mHolder.mDeviceContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);

    std::for_each(mRenderTargets.begin(), mRenderTargets.end(), [](RenderTargets_t::value_type& p)
    {
        p.second->FreeResources();
    });

    mHolder.mDeviceContext->ClearState();
    mHolder.mDeviceContext->Flush();

    if (mHolder.mSwapChain)
    {
        HRESULT hr = mHolder.mSwapChain->ResizeBuffers(
            0,
            0,
            0,
            DXGI_FORMAT_UNKNOWN,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
        );
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            YAGET_ASSERT(false, "Look at this ResizeBuffers since DXGI_ERROR_DEVICE_REMOVED or DXGI_ERROR_DEVICE_RESET was returned.");
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            // TODO! I think all we need to do is to reinitialize this object and return
            //*this = Device(App());
            return;
        }
        else
        {
            YAGET_THROW_ON_RROR(hr, "Could not resize buffers");
        }

        CreateRenderTarget("BackBuffer", RenderTarget::kDefaultSize, RenderTarget::kDefaultSize);
    }

    mRenderStateCache.reset(new state::RenderStateCache);

    D3D11_VIEWPORT viewport = {};

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = winSize.x;
    viewport.Height = winSize.y;
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;
    mHolder.mDeviceContext->RSSetViewports(1, &viewport);

    mDefaultState.Request({ "DepthStencil@DepthLess" }, { "Rasterizers@BackCull" }, { "Blends@Disabled" });
}

void yaget::render::Device::DefaultState::Request(const Section& depth, const Section& rasterizer, const Section& blend)
{
    YAGET_ASSERT(mDevice, "DefaultState does not have Device set to valid value.");

    io::Tag depthTag = mDevice->App().VTS().GetTag(depth);
    io::Tag rasterizerTag = mDevice->App().VTS().GetTag(rasterizer);
    io::Tag blendTag = mDevice->App().VTS().GetTag(blend);

    if (depthTag.IsValid())
    {
        mDepthRefreshId = idspace::get_burnable(mDevice->App().IdCache);
        mDevice->RequestResourceView<state::DepthStencilStateResource>(depthTag, std::ref(mDepth), mDepthRefreshId);
    }

    if (rasterizerTag.IsValid())
    {
        mRasterizerRefreshId = idspace::get_burnable(mDevice->App().IdCache);
        mDevice->RequestResourceView<state::RasterizerStateResource>(rasterizerTag, std::ref(mRasterizer), mRasterizerRefreshId);
    }

    if (blendTag.IsValid())
    {
        mBlendRefreshId = idspace::get_burnable(mDevice->App().IdCache);
        mDevice->RequestResourceView<state::BlendStateResource>(blendTag, std::ref(mBlend), mBlendRefreshId);
    }
}

bool yaget::render::Device::DefaultState::GetBlocks(std::vector<state::ResourcePtr>& defaultStates) const
{
    mt::SmartVariable<render::state::DepthStencilStateResource>::SmartType depth = mDepth;
    mt::SmartVariable<render::state::RasterizerStateResource>::SmartType rasterizer = mRasterizer;
    mt::SmartVariable<render::state::BlendStateResource>::SmartType blend = mBlend;

    if (depth && rasterizer && blend)
    {
        defaultStates = { depth, rasterizer, blend };
    }

    return !defaultStates.empty();
}

void yaget::render::Device::DefaultState::operator=(const DefaultState& source)
{
    mDevice = source.mDevice;
    YAGET_ASSERT(mDevice, "DefaultState does not have Device set to valid value.");

    mDevice->RemoveWatch(mDepthRefreshId);
    mDevice->RemoveWatch(mRasterizerRefreshId);
    mDevice->RemoveWatch(mBlendRefreshId);

    mDepth = source.mDepth;
    mRasterizer = source.mRasterizer;
    mBlend = source.mBlend;
    mDepthRefreshId = source.mDepthRefreshId;
    mRasterizerRefreshId = source.mRasterizerRefreshId;
    mBlendRefreshId = source.mBlendRefreshId;
}

yaget::render::Device::DefaultState::~DefaultState()
{
    YAGET_ASSERT(mDevice, "DefaultState does not have Device set to valid value.");

    mDevice->RemoveWatch(mDepthRefreshId);
    mDevice->RemoveWatch(mRasterizerRefreshId);
    mDevice->RemoveWatch(mBlendRefreshId);
}

yaget::render::Device::Holder::~Holder()
{
    mDeviceContext->ClearState();
    mDeviceContext->Flush();
}

void yaget::render::Device::DebugReport()
{
    if (dev::CurrentConfiguration().mGraphics.mMemoryReport && Device::DebugInterface)
    {
        Device::DebugInterface->ReportLiveDeviceObjects(/*D3D11_RLDO_SUMMARY*/D3D11_RLDO_DETAIL);
        Device::DebugInterface = nullptr;
    }
}

uint64_t yaget::render::Device::GetSignature() const
{
    return mSignature;
}

yaget::render::RenderTarget* yaget::render::Device::CreateRenderTarget(const std::string& name, uint32_t width, uint32_t height)
{
    RenderTargets_t::iterator it = mRenderTargets.find(name);
    if (it != mRenderTargets.end())
    {
        (*it).second->CreateResources(width, height);
        return (*it).second.get();
    }

    if (width == RenderTarget::kDefaultSize && height == RenderTarget::kDefaultSize)
    {
        mRenderTargets[name] = std::make_shared<RenderTarget>(*this, RenderTarget::kDefaultSize, RenderTarget::kDefaultSize, name);
    }
    else
    {
        mRenderTargets[name] = std::make_shared<RenderTarget>(*this, width, height, name);
    }

    return mRenderTargets[name].get();
}

render::RenderTarget* render::Device::FindRenderTarget(const std::string& name) const
{
    RenderTargets_t::const_iterator cit = mRenderTargets.find(name);
    return cit != mRenderTargets.end() ? (*cit).second.get() : nullptr;
}

//--------------------------------------------------------------------------------------------------
void yaget::render::Device::ResolveResourceRequest(const std::vector<io::Tag>& tags, const std::type_info& resourceType, ViewCallback viewCallback, ConvertCallback convertCallback, ResourceLoadRequest loadRequest)
{
    uint64_t sig = GetSignature();
    size_t hashCode = resourceType.hash_code();

    auto resolver = [this, viewCallback, convertCallback, sig, hashCode](std::shared_ptr<io::Asset> asset)
    {
        // using sig id to only trigger convert and view callbacks if request blob came within the same signature session
        uint64_t ThisSig = GetSignature();
        if (ThisSig != sig)
        {
            return;
        }

        try
        {
            {
                std::unique_lock<std::mutex> locker(mResourcesMutex);
                ResourceKey key = ResourceKey(asset->mTag.mGuid, hashCode);
                if (auto it = mViewResources.find(key); it != std::end(mViewResources))
                {
                    viewCallback(it->second);
                    return;
                }
            }

            std::shared_ptr<ResourceView> resourceView = convertCallback(asset);
            {
                std::unique_lock<std::mutex> locker(mResourcesMutex);

                ResourceKey key = ResourceKey(asset->mTag.mGuid, hashCode);
                mViewResources.insert(std::make_pair(key, resourceView));
            }

            viewCallback(resourceView);
        }
        catch (const ex::standard& e)
        {
            YLOG_ERROR("DEVV", "Did not convert Asset: '%s' to Resource. Error: %s", asset->mTag.ResolveVTS().c_str(), e.what());
        }
    };

    if (loadRequest == ResourceLoadRequest::Immediate)
    {
        io::BLobLoader<io::Asset> blobLoader(App().VTS(), tags);

        for (const auto& asset : blobLoader.Assets())
        {
            resolver(asset);
        }
    }
    else
    {
        App().VTS().RequestBlob(tags, resolver, nullptr);
    }
}
       
//--------------------------------------------------------------------------------------------------
void yaget::render::Device::RequestResourceView(const std::vector<io::Tag>& tags, const std::type_info& resourceType, ViewCallback viewCallback, ConvertCallback convertCallback, uint64_t watchIt, ResourceLoadRequest loadRequest)
{
    YAGET_ASSERT(viewCallback, "ViewCallback required for requested resource.");
    YAGET_ASSERT(convertCallback, "ConvertCallback required for requested resource.");
    YAGET_ASSERT(!tags.empty(), "Requested tags list is empty.");

    if (watchIt)
    {
        auto watchCallback = [tags, &resourceType, viewCallback, convertCallback, loadRequest, this]()
        {
            {
                auto rHash = resourceType.hash_code();
                std::unique_lock<std::mutex> locker(mResourcesMutex);

                for (const auto& tag : tags)
                {
                    ResourceKey key = ResourceKey(tag.mGuid, rHash);
                    mViewResources.erase(key);
                }
            }

            App().VTS().ClearAssets(tags);

            // We don't want to activate watch, since owner of this resource, will re-register watch.
            const uint64_t DoNotWatch = 0;
            RequestResourceView(tags, resourceType, viewCallback, convertCallback, DoNotWatch, loadRequest);
        };

        for (const auto& it : tags)
        {
            mResourceWatcher.Add(watchIt, it.ResolveVTS(), watchCallback);
        }
    }

    std::vector<io::Tag> neededTags;
    std::vector<std::shared_ptr<ResourceView>> foundAssets;
    {
        std::unique_lock<std::mutex> locker(mResourcesMutex);
        for (const auto& tag : tags)
        {
            YAGET_ASSERT(tag.mGuid.IsValid(), "Requested resource view (GUID/VTS): '%s/%s' is invalid.", tag.mGuid.str().c_str(), tag.ResolveVTS().c_str());

            ResourceKey key = ResourceKey(tag.mGuid, resourceType.hash_code());
            auto it = mViewResources.find(key);
            if (it != mViewResources.end())
            {
                foundAssets.push_back(it->second);
            }
            else
            {
                neededTags.push_back(tag);
            }
        }
    }

    for (const auto& it : foundAssets)
    {
        viewCallback(it);
    }

    if (!neededTags.empty())
    {
        ResolveResourceRequest(neededTags, resourceType, viewCallback, convertCallback, loadRequest);
    }
}

void yaget::render::Device::RemoveWatch(uint64_t watchIt)
{
    if (watchIt)
    {
        mResourceWatcher.Remove(watchIt);
    }
}

yaget::render::Device::ConvertorFunction yaget::render::Device::GetConvertor(const io::Tag& tag, const std::type_info& resourceType) const
{
    const std::string& resolverType = App().VTS().GetResolverType(tag);

    ResolverKey key(resolverType, std::type_index(resourceType));
    auto it = mTagResolvers.find(key);
    return it != mTagResolvers.end() ? it->second : nullptr;
}
