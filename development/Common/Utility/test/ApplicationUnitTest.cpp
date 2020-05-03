#include "UnitTest++.h"
#include "App/Application.h"
#include "VTS/VirtualTransportSystem.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"
#include "Items/ItemsDirector.h"


namespace
{
    yaget::io::VirtualTransportSystem::AssetResolvers Resolvers = {
        { "TEST", &yaget::io::ResolveAsset<yaget::io::JsonAsset> },
        { "JSON", &yaget::io::ResolveAsset<yaget::io::JsonAsset> }
    };

    class TestApplication : public yaget::Application
    {
    public:
        TestApplication(const std::string& title, yaget::items::Director& director, yaget::io::VirtualTransportSystem& vts, const yaget::args::Options& options)
            : yaget::Application(title, director, vts, options)
        {
        }

        virtual ~TestApplication() {}

        math3d::Vector2 GetWindowSize() const override { return math3d::Vector2(1, 1); }
        const yaget::time::GameClock& GameClock() const { return mGameClock; }

    private:
        bool onMessagePump(const yaget::time::GameClock&) override { return !mQuit; }
        void Cleanup() override {}
    };

} // namespace


TEST(ApplicationRun)
{
    using namespace yaget;

    io::tool::VirtualTransportSystemReset vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, "");
    items::Director director("$(DatabaseFolder)/items_test.sqlite");
    args::Options options("YagetCore.UnitTest", "Unit test of yaget core library");
    TestApplication app("UT.Application Run", director, vts, options);

    // let app run for 0.2 seconds and verity that game clock was updated enough times and it took appropriate time to finish
    int64_t logicCounter = 0;
    auto gameLogicLoop = [&logicCounter](Application& /*app*/, const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/)
    {
        logicCounter++;
    };

    int64_t renderCounter = 0;
    auto renderLoop = [&renderCounter](Application& /*app*/, const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/)
    {
        renderCounter++;
        platform::BusySleep(time::GetDeltaTime(dev::CurrentConfiguration().mInit.LogicTick), time::kMicrosecondUnit);
    };

    time::Microsecond_t startTime = platform::GetRealTime(time::kMicrosecondUnit);
    time::Microsecond_t quitTime = startTime + time::FromTo<time::Microsecond_t>(0.2f, time::kSecondUnit, time::kMicrosecondUnit);

    time::Microsecond_t logicStart = 0;
    auto idleLoop = [quitTime, &app, &logicStart]()
    {
        time::Microsecond_t currentTime = platform::GetRealTime(time::kMicrosecondUnit);
        if (logicStart == 0)
        {
            logicStart = currentTime;
        }
        if (currentTime >= quitTime)
        {
            app.RequestQuit();
        }
    };

    time::Microsecond_t logicEnd = 0;
    auto quitCallback = [&logicEnd]()
    {
        logicEnd = platform::GetRealTime(time::kMicrosecondUnit);
    };

    app.Run(gameLogicLoop, renderLoop, idleLoop, quitCallback);

    time::Microsecond_t diffLogic = logicEnd - logicStart;
    const time::Microsecond_t currentDeltaTime = time::GetDeltaTime(dev::CurrentConfiguration().mInit.LogicTick);
    int64_t tickCounter = static_cast<int64_t>(app.GameClock().GetTickCounter());

    CHECK_EQUAL(tickCounter, logicCounter);
    CHECK_CLOSE(logicCounter, renderCounter, 2);
    CHECK_CLOSE(diffLogic, tickCounter * currentDeltaTime, currentDeltaTime * 2);
}
