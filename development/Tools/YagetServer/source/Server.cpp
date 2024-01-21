#include "Server.h"
#include "ServerCoordinator.h"

#include "App/ConsoleApplication.h"
#include "Items/ItemsDirector.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"

#include <boost/asio.hpp>
#include <iostream>


int yaget::server::Run(const yaget::args::Options& options)
{
    const io::VirtualTransportSystem::AssetResolvers resolvers = {
        { "JSON", io::ResolveAsset<yaget::io::JsonAsset> }
    };

    const auto& configInitBlock = dev::CurrentConfiguration().mInit;
    const auto& vtsConfig = configInitBlock.mVTSConfig;
    io::tool::VirtualTransportSystemDefault vts(vtsConfig, resolvers);


    // for now we always reset Director while changes to schema are WIP
    items::BlankDefaultDirector director(vts, "Director", items::Director::RuntimeMode::Reset);
    app::DefaultConsole app("Yaget.Server", director, vts, options);
    Messaging messaging{};

    return comp::gs::RunGame<ServerSystemsCoordinator>(messaging, app);
    //return app.Run([](const time::GameClock& /*clock*/, metrics::Channel& /*channel*/)
    //{
    //    //using boost::asio::ip::tcp;

    //    //boost::asio::io_context io_context;

    //});
}
