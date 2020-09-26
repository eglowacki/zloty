#include "UnitTest++.h"
#include "YagetCore.h"
//#include "Exception/Exception.h"
#include "Device.h"
#include "VTS/ToolVirtualTransportSystem.h"
#include "VTS/DefaultResolvers.h"
#include "VTS/RenderStateResolvedAssets.h"
#include "App/ConsoleApplication.h"
#include "Items/ItemsDirector.h"

//#include "Exception/Exception.h"

//#define UNITTEST_CHECK_EQUAL(expected, actual)                                                                                                                \

TEST(IoAssets_Init)
{
    using namespace yaget;
    using Section = io::tool::VirtualTransportSystemDefault::Section;

    io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, io::tool::GetResolvers(), "$(DatabaseFolder)/vts.sqlite");

    io::Tag assetTag = vts.GenerateTag({ "TestAsset@Depth" });
    auto assetDepth = std::make_shared<io::render::DepthStencilStateAsset>(assetTag, io::Buffer(), vts);
    CHECK(assetDepth->IsValid());

    assetTag = vts.GenerateTag({ "TestAsset@DBlend" });
    auto assetBlend = std::make_shared<io::render::BlendStateAsset>(assetTag, io::Buffer(), vts);
    CHECK(assetBlend->IsValid());

    assetTag = vts.GenerateTag({ "TestAsset@Raster" });
    auto assetRaster = std::make_shared<io::render::RasterizerStateAsset>(assetTag, io::Buffer(), vts);
    CHECK(assetRaster->IsValid());

    assetTag = vts.GenerateTag({ "TestAsset@Pak" });
    auto assetPak = std::make_shared<io::render::PakAsset>(assetTag, io::Buffer(), vts);
    CHECK(assetPak->IsValid());
    CHECK_EQUAL(0, assetPak->mBuffer.second);

    //assetTag = vts.GenerateTag({ "TestAsset@Geom" });
    //auto assetGeom = std::make_shared<io::render::GeomAsset>(assetTag, io::Buffer(), vts);
    //CHECK(assetGeom->IsValid());
}

TEST(Device_Init)
{
    using namespace yaget;

    items::Director director("$(DatabaseFolder)/items.sqlite", {}, Database::NonVersioned);
    args::Options options("Yaget.UT.Device_Init", "Device Initialization");

    io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, io::tool::GetResolvers(), "$(DatabaseFolder)/vts.sqlite");
    app::BlankApplication app("UT - Device Init", director, vts, options);

    //render::Device device(app, io::tool::GetTagResolvers());


}

