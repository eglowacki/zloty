#include "App/Application.h"
#include "Debugging/DevConfiguration.h"


yaget::Application::Application(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options)
    : Options(options)
    , IdCache(&director)
    , mDirector(director)
    , mInputDevice(vts)
    , mGeneralPoolThread(std::make_unique<mt::JobPool>("AppPool", yaget::dev::CurrentConfiguration().mDebug.mThreads.App))
    , mVTS(vts)
{
    YLOG_INFO("INIT", "Created Application '%s'.", title.c_str());
}

void yaget::Application::onRenderTask(const yaget::Application::UpdateCallback_t& renderCallback)
{
    dev::CurrentThreadIds().RefreshRender(platform::CurrentThreadId());
    metrics::MarkStartThread(dev::CurrentThreadIds().Render, "RENDER");

    metrics::Channel channel("RenderThread", YAGET_METRICS_CHANNEL_FILE_LINE);

    while (!mQuit)
    {
        metrics::Channel rChannel("RenderTick", YAGET_METRICS_CHANNEL_FILE_LINE);

        renderCallback(*this, mApplicationClock, rChannel);
        std::this_thread::yield();
    }

    dev::CurrentThreadIds().RefreshRender(0);
}

void yaget::Application::onLogicTask(const UpdateCallback_t& logicCallback, const UpdateCallback_t& shutdownLogicCallback)
{
    dev::CurrentThreadIds().RefreshLogic(platform::CurrentThreadId());
    metrics::MarkStartThread(dev::CurrentThreadIds().Logic, "LOGIC");


    metrics::Channel channel("GameThread", YAGET_METRICS_CHANNEL_FILE_LINE);

    const time::Microsecond_t kFixedDeltaTime = time::GetDeltaTime(dev::CurrentConfiguration().mInit.LogicTick);
    metrics::PerformancePolicy defaultPerformancePolicy;

    mApplicationClock.Resync();
    time::Microsecond_t startTime = platform::GetRealTime(time::kMicrosecondUnit);
    time::Microsecond_t currentTickTime = startTime;
    time::Microsecond_t tickAccumulator = 0;

    using State = double;

    State previousState(0);
    State currentState(0);

    do
    {
        const time::Microsecond_t nextTickTime = platform::GetRealTime(time::kMicrosecondUnit);
        const time::Microsecond_t deltaTickTime = nextTickTime - currentTickTime;
        tickAccumulator += deltaTickTime;

        while (tickAccumulator >= kFixedDeltaTime)
        {
            metrics::Channel gChannel("GameTick", YAGET_METRICS_CHANNEL_FILE_LINE);

            const time::Microsecond_t startProcessTime = platform::GetRealTime(time::kMicrosecondUnit);
            const uint64_t tickCounter = mApplicationClock.GetTickCounter();

            mApplicationClock.Tick(kFixedDeltaTime);
            /*uint32_t numInputs =*/ mInputDevice.Tick(mApplicationClock, defaultPerformancePolicy, gChannel);

            tickAccumulator -= kFixedDeltaTime;

            if (logicCallback)
            {
                metrics::Channel gChannel("Callback", YAGET_METRICS_CHANNEL_FILE_LINE);

                logicCallback(*this, mApplicationClock, gChannel);
            }

            const time::Microsecond_t actualProcessTime = platform::GetRealTime(time::kMicrosecondUnit) - startProcessTime;
            if (actualProcessTime > kFixedDeltaTime)
            {
                YLOG_NOTICE("PROF", "Tick Loop tool too long. Budget: '%d' (mc), Actual: '%d' (mc).", kFixedDeltaTime, actualProcessTime);
                if (platform::IsDebuggerAttached())
                {
                    // since we are under debugger, we just adjust the main timer to account for that loss time (maybe due to break point)
                    platform::AdjustDrift(actualProcessTime - kFixedDeltaTime, time::kMicrosecondUnit);
                    YLOG_NOTICE("PROF", "Adjusted Main Real Time by: '%d' (mc).", actualProcessTime - kFixedDeltaTime);
                }
            }

            // if we need to quit, exit even if there are still some tickAccumulator left
            if (mQuit)
            {
                break;
            }

            metrics::Tick();
        }

        const double alpha = tickAccumulator / static_cast<double>(kFixedDeltaTime);

        State state = currentState * alpha + previousState * (1.0 - alpha);
        state;

        currentTickTime = nextTickTime;
        std::this_thread::yield();

    } while (!mQuit);

    if (shutdownLogicCallback)
    {
        shutdownLogicCallback(*this, mApplicationClock, channel);
    }

    dev::CurrentThreadIds().RefreshLogic(0);
}

int yaget::Application::Run(const TickLogic& tickLogic, const TickRender& tickRender /*= nullptr*/, const TickIdle& tickIdle /*= nullptr*/)
{
    auto tickWrapper = [this, &tickLogic](Application&, const time::GameClock& gameClock, metrics::Channel& channel) { tickLogic(gameClock, channel); };
    UpdateCallback_t renderWrapper = [this, &tickRender](Application&, const time::GameClock& gameClock, metrics::Channel& channel) { tickRender(gameClock, channel); };
    if (!tickRender)
    {
        renderWrapper = nullptr;
    }

    return Run(tickWrapper, nullptr, renderWrapper, tickIdle, nullptr);
}

int yaget::Application::Run(const UpdateCallback_t& logicCallback, const UpdateCallback_t& shutdownLogicCallback, const UpdateCallback_t& renderCallback, const StatusCallback_t& idleCallback, const StatusCallback_t& quitCallback)
{
    // kick off separate thread for rendering
    YAGET_ASSERT(mGeneralPoolThread, "Can not call Run second time.");
    if (renderCallback)
    {
        mGeneralPoolThread->AddTask([this, renderCallback]() { onRenderTask(renderCallback); });
    }
    mGeneralPoolThread->AddTask([this, &logicCallback, shutdownLogicCallback]() { onLogicTask(logicCallback, shutdownLogicCallback); });

    YLOG_DEBUG("APP", "Application.Run pump started.");
    while (!mRequestQuit)
    {
        //metrics::Channel channel("Message Pump", YAGET_METRICS_CHANNEL_FILE_LINE);

        onMessagePump(mApplicationClock);
        if (idleCallback)
        {
            idleCallback();
        }

        std::this_thread::yield();
    }
    YLOG_DEBUG("APP", "Application.Run pump ended.");

    // cleanup all threads here, wait for all completion before proceeding
    mQuit = true;

    if (quitCallback)
    {
        quitCallback();
    }

    mGeneralPoolThread.reset();
    YLOG_DEBUG("APP", "Application.Run mGeneralPoolThread stopped and cleared.");
    Cleanup();
    while (onMessagePump(mApplicationClock))
        ;

    YLOG_DEBUG("APP", "Application.Run returning back to caller!");
    return 0;
}

void yaget::Application::RequestQuit()
{
    mRequestQuit = true;
}

void yaget::Application::ChangeVideoSettings(const VideoOptions& /*videoOptions*/)
{
}

