#include "MainGame.h"

#include <Debugging/DevConfiguration.h>
#include "DefensorGameTypes.h"
#include "DefensorGameCoordinator.h"
#include "DefensorRenderCoordinator.h"
#include "Items/ItemsDirector.h"
#include "Render/DesktopApplication.h"
#include "VTS/DiagnosticVirtualTransportSystem.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"
#include "Render/AdapterInfo.h"
#include "Script/luacpp.h"
#include "../resource.h"

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


namespace 
{
    yaget::io::Tag AttachTransientAsset(yaget::render::AssetCacheType assetCacheType, yaget::io::VirtualTransportSystem& vts)
    {
        using namespace yaget;

        auto sigSection = render::AssetCache::operator[](assetCacheType);
        auto tag = vts.GenerateTag(sigSection);
        std::shared_ptr<io::Asset> newAsset = io::ResolveAsset<io::BinAsset>({}, tag, vts);
        vts.AttachTransientBlob(newAsset);

        return tag;
    }

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
        const std::string mSectionName;
    };

    std::vector<MappingPopulator> MappingPopulators = 
    {
        {&yaget::render::AssetCache::PopulateTypeToSection, &yaget::render::AssetCache::SaveTypeToSection, "Manifest@TypeToSection"},
        {&defensor::render::RenderShaders::PopulateShaderMappings, &defensor::render::RenderShaders::SaveShaderMappings, "Manifest@ShaderCompileOptions"},
    };

    struct Mappers
    {
        Mappers(yaget::io::VirtualTransportSystem& vts)
            : mVTS(vts)
        {
            AttachTransientAsset(yaget::render::BasicSignature, mVTS);
            AttachTransientAsset(yaget::render::BasicPipeline, mVTS);

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
        yaget::io::diag::VirtualTransportSystem vtsFixer(false, "$(DatabaseFolder)/vts.sqlite");
    }

    auto msgTextLine = comp::db::GenerateSystemsCoordinator<game::DefensorSystemsCoordinator, comp::db::GenerateCoordinator::Log>() +"\n\t";
    msgTextLine += comp::db::GenerateSystemsCoordinator<render::DefensorSystemsCoordinator, comp::db::GenerateCoordinator::Log>();
    YLOG_INFO("DEF", msgTextLine.c_str());

    const io::VirtualTransportSystem::AssetResolvers resolvers = {
        { "JSON", io::ResolveAsset<io::JsonAsset> },
        { "COMP", io::ResolveAsset<io::StringsAsset> },
        { "PERS", io::ResolveAsset<io::StringsAsset> },
        { "BIN", io::ResolveAsset<io::BinAsset> }
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

    auto returnResult = comp::gs::RunGame<game::DefensorSystemsCoordinator, render::DefensorSystemsCoordinator>(messaging, app);
    return returnResult;
}
