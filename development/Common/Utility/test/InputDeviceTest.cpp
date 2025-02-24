#include "UnitTest++.h"
#include "Input/InputDevice.h"
#include "VTS/ToolVirtualTransportSystem.h"
#include "VTS/ResolvedAssets.h"
#include "Metrics/Concurrency.h"

namespace
{
    //-------------------------------------------------------------------------------------------------------------------------------
    yaget::io::VirtualTransportSystem::AssetResolvers Resolvers = {
    { "TEST", &yaget::io::ResolveAsset<yaget::io::JsonAsset> },
    { "JSON", &yaget::io::ResolveAsset<yaget::io::JsonAsset> }
    };

    const char* j2 = R"(
    {
        "happy": true,
        "pi": 3.141
    })";


    const char* keyBindings = " \
    { \
        \"Trigger1\": \
        { \
            \"Action\": \"UT.Trigger1\", \
                \"ContextName\" : \"UNIT_TEST\", \
                \"DisplayName\" : \"T1\", \
                \"Flags\" : \"ButtonDown\", \
                \"Value\" : \"1\" \
        }, \
        \
        \"Trigger2\": \
        { \
            \"Action\": \"UT.Trigger2\", \
                \"ContextName\" : \"UNIT_TEST2\", \
                \"DisplayName\" : \"T2\", \
                \"Flags\" : \"ButtonDown\", \
                \"Value\" : \"2\" \
        } \
    }";

    const char* mouseBindings = " \
    { \
        \"Mouse1\": \
        { \
            \"Action\": \"UT.Mouse1\", \
                \"ContextName\" : \"UNIT_TEST\", \
                \"DisplayName\" : \"M1\", \
                \"Flags\" : \"ButtonDown\", \
                \"Value\" : \"MouseLeft\" \
        }, \
        \
        \"Mouse2\": \
        { \
            \"Action\": \"UT.Mouse2\", \
                \"ContextName\" : \"UNIT_TEST\", \
                \"DisplayName\" : \"M2\", \
                \"Flags\" : \"ButtonDown\", \
                \"Value\" : \"MouseRight\" \
        } \
    }";

    void TriggerCallback(int action, bool results[], uint64_t ticks[], const yaget::time::GameClock& gameClock)
    {
        results[action] = true;
        ticks[action] = gameClock.GetTickCounter();
    }

    yaget::io::tool::VirtualTransportSystem::Section AddKeyBindingsFile(const std::string& text)
    {
        using namespace yaget;
        using Section = io::tool::VirtualTransportSystem::Section;
        const Section overrideSettings("OverrideSettings");

        io::tool::VirtualTransportSystemReset vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, "");
        CHECK(vts.DeleteBlob(overrideSettings));

        const Section settingsFile("OverrideSettings@Override/KeyBindings.json");
        io::Tag tag = vts.GenerateTag(settingsFile);
        CHECK(tag.IsValid());
        io::Buffer data = io::CreateBuffer(text);
        CHECK(vts.AttachBlob(std::make_shared<io::JsonAsset>(tag, data, vts)));

        return overrideSettings;
    }

} // namespace


TEST(InputDevice)
{
    using namespace yaget;
    using Section = io::tool::VirtualTransportSystem::Section;
    const Section overrideSettings = AddKeyBindingsFile(keyBindings);

    metrics::PerformancePolicy defaultPerformancePolicy;
    time::GameClock gameClock;
    bool results[] = { false, false, false };
    uint64_t ticks[] = { std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max() };
    const uint64_t kTick1 = 5;
    const uint64_t kTick2 = 9;

    io::tool::VirtualTransportSystemReset vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, "");
    input::InputDevice inputDevice(vts);

    inputDevice.RegisterSimpleActionCallback("UT.Trigger1", [&]() { TriggerCallback(1, results, ticks, std::cref(gameClock)); });
    inputDevice.RegisterSimpleActionCallback("UT.Trigger2", [&]() { TriggerCallback(2, results, ticks, std::cref(gameClock)); });

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    // check if context push and pop
    CHECK_EQUAL("", inputDevice.PopContext());
    inputDevice.PushContext("DUMMY_CONTEXT_A");
    inputDevice.PushContext("DUMMY_CONTEXT_B");
    CHECK_EQUAL("DUMMY_CONTEXT_B", inputDevice.PopContext());
    CHECK_EQUAL("DUMMY_CONTEXT_A", inputDevice.PopContext());
    CHECK_EQUAL("", inputDevice.PopContext());
    CHECK_EQUAL("", inputDevice.PopContext());

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    // check if input is triggered with empty context (*)
    metrics::Channel channel("InputTest");

    gameClock.Resync();
    time::Microsecond_t baseTime = gameClock.GetLogicTime();
    inputDevice.KeyRecord(input::kButtonDown, '1', baseTime + (time::kDeltaTime_60 * kTick1));
    inputDevice.KeyRecord(input::kButtonDown, '2', baseTime + (time::kDeltaTime_60 * kTick2));

    for (int i = 0; i < time::kFrames_60; ++i)
    {
        inputDevice.Tick(gameClock, defaultPerformancePolicy, channel);
        gameClock.Tick(time::kDeltaTime_60);
    }

    CHECK(results[1]);
    CHECK(results[2]);
    CHECK_EQUAL(kTick1, ticks[1]);
    CHECK_EQUAL(kTick2, ticks[2]);

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    // check if input is triggered with correct context (UNIT_TEST)
    inputDevice.PushContext("UNIT_TEST");
    results[0] = false;
    results[1] = false;
    results[2] = false;
    ticks[0] = std::numeric_limits<uint64_t>::max();
    ticks[1] = std::numeric_limits<uint64_t>::max();
    ticks[2] = std::numeric_limits<uint64_t>::max();

    gameClock.Resync();
    baseTime = gameClock.GetLogicTime();
    inputDevice.KeyRecord(input::kButtonDown, '1', baseTime + (time::kDeltaTime_60 * kTick1));
    inputDevice.KeyRecord(input::kButtonDown, '2', baseTime + (time::kDeltaTime_60 * kTick2));

    for (int i = 0; i < time::kFrames_60; ++i)
    {
        inputDevice.Tick(gameClock, defaultPerformancePolicy, channel);
        gameClock.Tick(time::kDeltaTime_60);
    }

    CHECK(results[1]);
    CHECK(results[2] == false);
    CHECK_EQUAL(kTick1, ticks[1]);
    CHECK_EQUAL(std::numeric_limits<uint64_t>::max(), ticks[2]);

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    // check if input is NOT triggered with wrong context (UNIT_TEST_DUMMY)
    inputDevice.PushContext("UNIT_TEST_DUMMY");
    results[0] = false;
    results[1] = false;
    results[2] = false;
    ticks[0] = std::numeric_limits<uint64_t>::max();
    ticks[1] = std::numeric_limits<uint64_t>::max();
    ticks[2] = std::numeric_limits<uint64_t>::max();

    gameClock.Resync();
    baseTime = gameClock.GetLogicTime();
    inputDevice.KeyRecord(input::kButtonDown, '1', baseTime + (time::kDeltaTime_60 * kTick1));
    inputDevice.KeyRecord(input::kButtonDown, '2', baseTime + (time::kDeltaTime_60 * kTick2));

    for (int i = 0; i < time::kFrames_60; ++i)
    {
        inputDevice.Tick(gameClock, defaultPerformancePolicy, channel);
        gameClock.Tick(time::kDeltaTime_60);
    }

    CHECK(results[1] == false);
    CHECK(results[2] == false);
    CHECK_EQUAL(std::numeric_limits<uint64_t>::max(), ticks[1]);
    CHECK_EQUAL(std::numeric_limits<uint64_t>::max(), ticks[2]);

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    // check if input is triggered with correct context (UNIT_TEST) after pop
    inputDevice.PopContext();
    results[0] = false;
    results[1] = false;
    results[2] = false;
    ticks[0] = std::numeric_limits<uint64_t>::max();
    ticks[1] = std::numeric_limits<uint64_t>::max();
    ticks[2] = std::numeric_limits<uint64_t>::max();

    gameClock.Resync();
    baseTime = gameClock.GetLogicTime();
    inputDevice.KeyRecord(input::kButtonDown, '1', baseTime + (time::kDeltaTime_60 * kTick1));
    inputDevice.KeyRecord(input::kButtonDown, '2', baseTime + (time::kDeltaTime_60 * kTick2));

    for (int i = 0; i < time::kFrames_60; ++i)
    {
        inputDevice.Tick(gameClock, defaultPerformancePolicy, channel);
        gameClock.Tick(time::kDeltaTime_60);
    }

    CHECK(results[1]);
    CHECK(results[2] == false);
    CHECK_EQUAL(kTick1, ticks[1]);
    CHECK_EQUAL(std::numeric_limits<uint64_t>::max(), ticks[2]);

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    inputDevice.PopContext();
    // check if input is triggered with global context (*) after last pop
    inputDevice.PopContext();
    results[0] = false;
    results[1] = false;
    results[2] = false;
    ticks[0] = std::numeric_limits<uint64_t>::max();
    ticks[1] = std::numeric_limits<uint64_t>::max();
    ticks[2] = std::numeric_limits<uint64_t>::max();

    gameClock.Resync();
    baseTime = gameClock.GetLogicTime();
    inputDevice.KeyRecord(input::kButtonDown, '1', baseTime + (time::kDeltaTime_60 * kTick1));
    inputDevice.KeyRecord(input::kButtonDown, '2', baseTime + (time::kDeltaTime_60 * kTick2));

    for (int i = 0; i < time::kFrames_60; ++i)
    {
        inputDevice.Tick(gameClock, defaultPerformancePolicy, channel);
        gameClock.Tick(time::kDeltaTime_60);
    }

    CHECK(results[1]);
    CHECK(results[2]);
    CHECK_EQUAL(kTick1, ticks[1]);
    CHECK_EQUAL(kTick2, ticks[2]);

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    // check if input responds to TriggerAction
    results[0] = false;
    results[1] = false;
    results[2] = false;
    ticks[0] = std::numeric_limits<uint64_t>::max();
    ticks[1] = std::numeric_limits<uint64_t>::max();
    ticks[2] = std::numeric_limits<uint64_t>::max();

    inputDevice.TriggerAction("UT.Trigger1", 0, 0);

    CHECK(results[1]);
    CHECK(vts.DeleteBlob(overrideSettings));

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    // let's check multi-threaded functionality
    results[0] = false;
    results[1] = false;
    results[2] = false;
    ticks[0] = std::numeric_limits<uint64_t>::max();
    ticks[1] = std::numeric_limits<uint64_t>::max();
    ticks[2] = std::numeric_limits<uint64_t>::max();

    std::pair<uint64_t, uint64_t> deltaTicks[time::kFrames_60];

    mt::JobPool jobPool("InputMaker", 1);
    std::atomic_bool quit{ false };

    const time::Microsecond_t epochTime = platform::GetRealTime(time::kMicrosecondUnit);
    gameClock.Resync();
    uint32_t inputMessageCounter = 0;

    jobPool.AddTask([&inputDevice, &quit, epochTime, &inputMessageCounter]()
    {
        while (quit == false)
        {
            inputMessageCounter++;
            time::Microsecond_t currentTime = platform::GetRealTime(time::kMicrosecondUnit);
            inputDevice.KeyRecord(input::kButtonDown, '1', currentTime);

            if (currentTime - epochTime > time::FromTo<time::Microsecond_t>(0.75f, time::kSecondUnit, time::kMicrosecondUnit))
            {
                quit = true;
            }

            std::this_thread::yield();
        }
    });

    time::Microsecond_t startTime = platform::GetRealTime(time::kMicrosecondUnit);
    time::Microsecond_t desiredEndTime = startTime + time::FromTo<time::Microsecond_t>(1.0f, time::kSecondUnit, time::kMicrosecondUnit);
    time::Microsecond_t currentTickTime = startTime;
    time::Microsecond_t tickAccumulator = 0;
    uint32_t numTotalInputsProcessed = 0;

    do
    {
        time::Microsecond_t nextTickTime = platform::GetRealTime(time::kMicrosecondUnit);
        time::Microsecond_t deltaTickTime = nextTickTime - currentTickTime;
        tickAccumulator += deltaTickTime;

        while (tickAccumulator >= time::kDeltaTime_60)
        {
            metrics::Channel span("FrameTick");

            time::Microsecond_t startProccessTime = platform::GetRealTime(time::kMicrosecondUnit);
            const uint64_t tickCounter = gameClock.GetTickCounter();

            gameClock.Tick(time::kDeltaTime_60);
            uint32_t numInputs = inputDevice.Tick(gameClock, defaultPerformancePolicy, channel);
            numTotalInputsProcessed += numInputs;

            tickAccumulator -= time::kDeltaTime_60;

            time::Microsecond_t endProccessTime = platform::GetRealTime(time::kMicrosecondUnit);
            deltaTicks[tickCounter].first = numInputs;
            deltaTicks[tickCounter].second = endProccessTime - startProccessTime;

            // if we need to quit, exit even if there are still some tickAccumulator left
            if (!(nextTickTime < desiredEndTime && gameClock.GetTickCounter() < time::kFrames_60))
            {
                break;
            }
        }

        currentTickTime = nextTickTime;

    } while (currentTickTime < desiredEndTime && gameClock.GetTickCounter() < time::kFrames_60);

    quit = true;

    CHECK_EQUAL(inputMessageCounter, numTotalInputsProcessed);

    time::Microsecond_t endTime = platform::GetRealTime(time::kMicrosecondUnit);
    time::Microsecond_t runTime = endTime - startTime;
    time::Milisecond_t differenceTime = time::FromTo<time::Milisecond_t>(runTime, time::kMicrosecondUnit, time::kMilisecondUnit);
    differenceTime;
}


TEST(InputDevice_Mouse)
{
    using namespace yaget;
    using Section = io::tool::VirtualTransportSystem::Section;
    const Section overrideSettings = AddKeyBindingsFile(mouseBindings);

    io::tool::VirtualTransportSystemReset vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, "");
    input::InputDevice inputDevice(vts);

    int mouseHit1 = 0;
    inputDevice.RegisterSimpleActionCallback("UT.Mouse1", [&mouseHit1]()
    {
        mouseHit1++;;
    });

    int mouseHit2 = 0;
    inputDevice.RegisterSimpleActionCallback("UT.Mouse2", [&mouseHit2]()
    {
        mouseHit2++;;
    });

    metrics::PerformancePolicy defaultPerformancePolicy;
    metrics::Channel channel("InputTest_Mouse");

    time::GameClock gameClock;
    time::Microsecond_t baseTime = gameClock.GetLogicTime();

    // test mouse buttons down (left, right)
    input::InputDevice::Mouse::Buttons buttons;
    buttons[input::kMouseLeft] = true;
    inputDevice.MouseRecord(input::kButtonDown, buttons, 0, input::InputDevice::Mouse::Location(0, 0), baseTime);

    buttons = input::InputDevice::Mouse::Buttons();
    buttons[input::kMouseRight] = true;

    inputDevice.MouseRecord(input::kButtonDown, buttons, 0, input::InputDevice::Mouse::Location(0, 0), baseTime + time::kDeltaTime_60);

    for (int i = 0; i < 3; ++i)
    {
        inputDevice.Tick(gameClock, defaultPerformancePolicy, channel);
        gameClock.Tick(time::kDeltaTime_60);
    }

    CHECK_EQUAL(1, mouseHit1);
    CHECK_EQUAL(1, mouseHit2);

    // test left and right mouse buttons down, this should not generate any input callback, since it does not match
    mouseHit1 = 0;
    mouseHit2 = 0;
    buttons = input::InputDevice::Mouse::Buttons();
    buttons[input::kMouseLeft] = true;
    buttons[input::kMouseRight] = true;

    gameClock.Resync();
    baseTime = gameClock.GetLogicTime();
    inputDevice.MouseRecord(input::kButtonDown, buttons, 0, input::InputDevice::Mouse::Location(0, 0), baseTime);
    inputDevice.Tick(gameClock, defaultPerformancePolicy, channel);

    CHECK_EQUAL(0, mouseHit1);
    CHECK_EQUAL(0, mouseHit2);

    CHECK(vts.DeleteBlob(overrideSettings));
}
