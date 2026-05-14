#include "MainGame.h"

#include "App/Splash.h"
#include "Debugging/DevConfiguration.h"
#include "DefensorGameCoordinator.h"
#include "DefensorGameTypes.h"
#include "DefensorRenderCoordinator.h"
#include "Items/ItemsDirector.h"
#include "Render/Cache/AssetCache.h"
#include "Render/DesktopApplication.h"
#include "Render/Pipeline/RenderMaterialProperties.h"
#include "Render/Pipeline/RenderPipelines.h"
#include "Render/Pipeline/RenderShaders.h"
#include "Render/Pipeline/RenderSignatures.h"
#include "Render/Pipeline/RenderTextures.h"
#include "Render/Scene/RenderSceneItems.h"
#include "Script/luacpp.h"
#include "Time/DeltaClock.h"
#include "VTS/DiagnosticVirtualTransportSystem.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"

#include "../resource.h"


#if 0
namespace yaget::app
{
   
    //template <typename VTS, typename D, typename A>
    //using ApplicationFramework = std::tuple<VTS, D, A>;

    template <typename VTS, typename D, typename S, typename A, typename M>
    int SetupApplicationFramework(const yaget::args::Options& options)
    {
        const io::VirtualTransportSystem::AssetResolvers resolvers = {
            { "JSON", io::ResolveAsset<io::JsonAsset> }
        };

        const auto& configInitBlock = dev::CurrentConfiguration().mInit;
        VTS vts(configInitBlock.mVTSConfig, resolvers);
        
        // we want to preserve Director DB content between runs, with option to re-initialize db to default values.
        const items::Director::RuntimeMode directorMode = options.find<bool>("director_fix", false) ? items::Director::RuntimeMode::Reset : items::Director::RuntimeMode::Default;
        D director(vts, "Director", directorMode);

        const auto selectedAdapter = yaget::render::info::SelectDefaultAdapter(configInitBlock.ResX, configInitBlock.ResY);
        A app("Yaget.Defensor", director, vts, options, selectedAdapter);

        //return std::move(app);
        M messaging{};

        return comp::gs::RunGame<S>(messaging, app);
    }
}
#endif

namespace 
{
#if 0
    static int l_log(lua_State* L) 
    {
        const char* msg = luaL_checkstring(L, 1);
        YLOG_INFO("DEF", "%s\n", msg);
        return 0; // Number of results
    }
#endif

    struct MappingPopulator
    {
        std::function<void(const yaget::io::VirtualTransportSystem::Section& sectionName, yaget::io::VirtualTransportSystem& vts)> mPopulator;
        std::function<void(const yaget::io::VirtualTransportSystem::Section& sectionName, yaget::io::VirtualTransportSystem& vts)> mSaver;
        std::string mSectionName;
    };

    std::vector<MappingPopulator> MappingPopulators = 
    {
        {&yaget::render::AssetCache::PopulateMappings, &yaget::render::AssetCache::SaveMappings, "Manifest@TypeToSection"},
        {&yaget::render::RenderSignatures::PopulateMappings, &yaget::render::RenderSignatures::SaveMappings, "Manifest@RenderSignatureOptions"},
        {&yaget::render::RenderPipelines::PopulateMappings, &yaget::render::RenderPipelines::SaveMappings, "Manifest@RenderPipelineOptions"},
        {&yaget::render::RenderShaders::PopulateMappings, &yaget::render::RenderShaders::SaveMappings, "Manifest@ShaderCompileOptions"},
        {&yaget::render::RenderShaders::PopulateReflectorMappings, &yaget::render::RenderShaders::SaveReflectorMappings, "Manifest@ShaderReflectionOptions"},
        {&yaget::render::RenderMaterialProperties::PopulateMappings, &yaget::render::RenderMaterialProperties::SaveMappings, "Manifest@RenderMaterialPropertyOptions"},
        {&yaget::render::RenderTextures::PopulateMappings, &yaget::render::RenderTextures::SaveMappings, "Manifest@RenderTextureOptions"},
        {&yaget::render::scene::SceneItemsStorage::PopulateMappings, &yaget::render::scene::SceneItemsStorage::SaveMappings, "Manifest@RenderSceneItemOptions"},
        {&yaget::render::commands::RenderTargetStorage::PopulateMappings, &yaget::render::commands::RenderTargetStorage::SaveMappings, "Manifest@RenderTargetOptions"},
    };

    struct Mappers
    {
        Mappers(yaget::io::VirtualTransportSystem& vts)
            : mVTS(vts)
        {
            for (const auto mapping : MappingPopulators)
            {
                mapping.mPopulator(mapping.mSectionName, mVTS);
            }
        }

        ~Mappers()
        {
            for (const auto mapping : MappingPopulators)
            {
                yaget::io::VirtualTransportSystem::Section populatorName = mapping.mSectionName;
                yaget::io::VirtualTransportSystem::Section saverName = populatorName.mName + "Write@" + populatorName.mFilter;
                mapping.mSaver(saverName, mVTS);
            }
        }

        yaget::io::VirtualTransportSystem& mVTS;
    };

    //-------------------------------------------------------------------------------------------------
    class SplashScreenUpdater : public yaget::NoCopy
    {
    public:
        SplashScreenUpdater(defensor::game::Messaging& messaging, yaget::Application& application, const std::string& fileName, COLORREF color = { 0x00000000 })
            : mMessaging(messaging)
            , mApplication(application)
            , mSplashWindow(yaget::util::ExpendEnv(fileName, nullptr), color)
        {
            mSplashWindow.ShowSplash();

            mInitEventHandle = mMessaging.Listen<yaget::comp::gs::InitEvent>([this](const auto& event)
            {
                {
                    std::scoped_lock lock(mInitCounterMutex);

                    mFinishedPreloading = mInitCounter && event.mItemsProcessed == nullptr;
                    mInitCounter = event.mItemsProcessed;

                    if (!event.mText.empty())
                    {
                        mProgressMessage = event.mText;
                    }
                }


                if (event.mNumItems != -1)
                {
                    mTotalItemsToPreload = event.mNumItems;
                }
            }, defensor::game::Messaging::DispatcherType::Logic);
        }


        //-------------------------------------------------------------------------------------------------
        void OnTick()
        {
            if (mFinishedPreloading)
            {
                if (mInitEventHandle)
                {
                    mMessaging.Remove(mInitEventHandle, defensor::game::Messaging::DispatcherType::Logic);
                    mInitEventHandle = 0;
                    mSplashWindow.CloseSplash();
                    mApplication.DisplayWindow(true);
                }

                return;
            }

            if (!mDeltaClock.IsDeltaTimePassed())
            {
                return;
            }

            int currentCounter = -1;
            std::string progressMessage;
            {
                std::scoped_lock lock(mInitCounterMutex);
                if (mInitCounter)
                {
                    currentCounter = mInitCounter->load();
                }

                progressMessage = mProgressMessage;
            }

            if (currentCounter != -1)
            {
                if (mLastPreloadCounter != currentCounter)
                {
                    mLastPreloadCounter = currentCounter;

                    auto counterMessage = std::format("{}/{}", mLastPreloadCounter, mTotalItemsToPreload.load());
                    mSplashWindow.Print(counterMessage.c_str(), yaget::Splash::TextLine::Second);
                }
            }

            if (progressMessage != mLastProgressMessage)
            {
                mLastProgressMessage = progressMessage;
                mSplashWindow.Print(mLastProgressMessage.c_str(), yaget::Splash::TextLine::First);
            }
        }


        //-------------------------------------------------------------------------------------------------
        ~SplashScreenUpdater()
        {
            if (mInitEventHandle)
            {
                mMessaging.Remove(mInitEventHandle, defensor::game::Messaging::DispatcherType::Logic);
                mSplashWindow.CloseSplash();
            }
        }

    private:
        defensor::game::Messaging& mMessaging;
        yaget::Application& mApplication;
        yaget::Splash mSplashWindow;
        uint64_t mInitEventHandle{ 0 };
        yaget::comp::gs::mt::InitCounter* mInitCounter{ nullptr };
        yaget::comp::gs::mt::InitCounter mTotalItemsToPreload{ -1 };
        std::atomic_bool mFinishedPreloading{ false };
        int32_t mLastPreloadCounter{ -1 };
        std::string mProgressMessage;
        std::string mLastProgressMessage;

        yaget::time::DeltaClock mDeltaClock{ 1.0/5.0 };
        std::mutex mInitCounterMutex;
    };

}


int defensor::Run(const yaget::args::Options& options)
{
    using namespace yaget;

#if 0
    lua_State* L = luaL_newstate(); // Create new Lua state
    luaL_openlibs(L);               // Load Lua libraries

    lua_pushcfunction(L, l_log);
    lua_setglobal(L, "log");

    // Execute Lua script
    if (luaL_dostring(L, "log('Hello from Lua!')")) 
    {
        printf("Error: %s\n", lua_tostring(L, -1));
    }

    lua_close(L); // Close Lua state
#endif

    if (options.find<bool>("vts_fix", false))
    {
        io::diag::VirtualTransportSystem vtsFixer(false, "$(DatabaseFolder)/vts.sqlite");
    }

    auto msgTextLine = comp::db::GenerateSystemsCoordinator<game::DefensorSystemsCoordinator, comp::db::GenerateCoordinator::Log>() +"\n\t";
    msgTextLine += comp::db::GenerateSystemsCoordinator<render::DefensorSystemsCoordinator, comp::db::GenerateCoordinator::Log>();
    YLOG_INFO("DEF", msgTextLine.c_str());

    const io::VirtualTransportSystem::AssetResolvers resolvers = {
        { "JSON", io::ResolveAsset<io::JsonAsset> },
        { "COMP", io::ResolveAsset<io::StringsAsset> },
        { "PERS", io::ResolveAsset<io::StringsAsset> },
        { "GEOM", io::ResolveAsset<io::StringsAsset> },
        { "BIN", io::ResolveAsset<io::BinAsset> },
        { "IMAGE", io::ResolveAsset<io::TextureAsset> }
    };

    const auto& configInitBlock = dev::CurrentConfiguration().mInit;
    io::tool::VirtualTransportSystemDefault vts(configInitBlock.mVTSConfig, resolvers);

    // we want to preserve Director DB content between runs, with option to re-initialize db to default values.
    const items::Director::RuntimeMode directorMode = options.find<bool>("director_fix", false) ? items::Director::RuntimeMode::Reset : items::Director::RuntimeMode::Default;
    items::DefaultDirector<game::DefensorSystemsCoordinator> director("Director", directorMode);

    yaget::render::DesktopApplication::IconId = IDI_ICON2;
    const auto selectedAdapter = yaget::render::info::SelectDefaultAdapter(configInitBlock.ResX, configInitBlock.ResY);
    yaget::render::DesktopApplication app("Yaget.Defensor", director, vts, options, selectedAdapter);

    game::Messaging messaging{};
    Mappers mappers(app.VTS());

    std::string splashBitmapName = "$(AppFolder)/Splash.bmp";
    SplashScreenUpdater splashScreenUpdater(messaging, app, splashBitmapName, COLORREF{ 0xCA000000 });

    auto returnResult = comp::gs::RunGame<game::DefensorSystemsCoordinator, render::DefensorSystemsCoordinator>(messaging, app, [&splashScreenUpdater]() { splashScreenUpdater.OnTick(); });
    return returnResult;
}
