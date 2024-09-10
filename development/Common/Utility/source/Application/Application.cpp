#include "App/Application.h"
#include "Debugging/DevConfiguration.h"
#include "Items/ItemsDirector.h"
#include "MemoryManager/NewAllocator.h"


//-------------------------------------------------------------------------------------------------
yaget::Application::Application(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options)
    : Options(options)
    , IdCache(director.IdCache())
    , mDirector(director)
    , mInputDevice(vts)
    , mGeneralPoolThread(std::make_unique<mt::JobPool>("AppPool", dev::CurrentConfiguration().mDebug.mThreads.App, mt::JobPool::Behaviour::StartAsPause))
    , mVTS(vts)
{
    YLOG_INFO("INIT", "Created Application '%s'.", title.c_str());
}


//-------------------------------------------------------------------------------------------------
void yaget::Application::onRenderTask(const Application::TickLogic& renderCallback)
{
    memory::RecordAllocations recordAllocations;

    dev::CurrentThreadIds().RefreshRender(platform::CurrentThreadId());
    metrics::MarkStartThread(dev::CurrentThreadIds().Render, "RENDER");

    metrics::Channel channel("RenderThread");

    YLOG_INFO("APP", "Started Render Task...");

    mRenderClock.Resync();
    time::Microsecond_t lastRenderTime = mRenderClock.GetRealTime();

    while (!mQuit)
    {
        FrameCounter::Collector fpsCollector(mRenderFrameCounter);

        metrics::Channel rChannel("RenderTick");

        if (IsSuspended())
        {
            metrics::Channel sChannel("Suspended");
            platform::Sleep([this] { return IsSuspended(); });
        }

        renderCallback(mRenderClock, rChannel);
        std::this_thread::yield();

        const time::Microsecond_t currentRenderTime = mRenderClock.GetRealTime();
        const time::Microsecond_t deltaTime = currentRenderTime - lastRenderTime;
        lastRenderTime = currentRenderTime;

        mRenderClock.Tick(deltaTime);
    }

    YLOG_INFO("APP", "Ended Render Task.");
    dev::CurrentThreadIds().RefreshRender(0);
}


//-------------------------------------------------------------------------------------------------
void yaget::Application::onLogicTask(const TickLogic& logicCallback, const TickLogic& shutdownLogicCallback)
{
    memory::RecordAllocations recordAllocations;

    dev::CurrentThreadIds().RefreshLogic(platform::CurrentThreadId());
    metrics::MarkStartThread(dev::CurrentThreadIds().Logic, "LOGIC");

    metrics::Channel channel("GameThread");

    YLOG_INFO("APP", "Started Logic Task...");

    const time::Microsecond_t kFixedDeltaTime = time::GetDeltaTime(dev::CurrentConfiguration().mInit.LogicTick);
    const metrics::PerformancePolicy defaultPerformancePolicy;

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
            FrameCounter::Collector fpsCollector(mLogicFrameCounter);

            metrics::Channel gameChannel("GameTick");

            const time::Microsecond_t startProcessTime = platform::GetRealTime(time::kMicrosecondUnit);

            /*uint32_t numInputs =*/ mInputDevice.Tick(mApplicationClock, defaultPerformancePolicy, gameChannel);

            tickAccumulator -= kFixedDeltaTime;

            if (logicCallback)
            {
                metrics::Channel channel("Callback");

                logicCallback(mApplicationClock, channel);
            }                                                                                                                                      

            mApplicationClock.Tick(kFixedDeltaTime);

            const time::Microsecond_t actualProcessTime = platform::GetRealTime(time::kMicrosecondUnit) - startProcessTime;
            if (actualProcessTime > kFixedDeltaTime)
            {
                YLOG_NOTICE("PROF", "Tick Loop tool too long. Budget: '%d' (mc), Actual: '%d' (mc).", kFixedDeltaTime, actualProcessTime);
                if (platform::IsDebuggerAttached())
                {
                    // since we are under debugger, we just adjust the main timer to account for that loss time (maybe due to break point)
                    platform::AdjustDrift(actualProcessTime - kFixedDeltaTime, time::kMicrosecondUnit);
                    mApplicationClock.Resync();
                    YLOG_NOTICE("PROF", "Adjusted Main Real Time by: '%d' (mc).", actualProcessTime - kFixedDeltaTime);
                }
            }

            // if we need to quit, exit even if there are still some tickAccumulator left
            if (mQuit)
            {
                break;
            }
        }

        const double alpha = tickAccumulator / static_cast<double>(kFixedDeltaTime);

        [[maybe_unused]] State state = currentState * alpha + previousState * (1.0 - alpha);

        currentTickTime = nextTickTime;
        std::this_thread::yield();

    } while (!mQuit);

    if (shutdownLogicCallback)
    {
        shutdownLogicCallback(mApplicationClock, channel);
    }

    YLOG_INFO("APP", "Ended Logic Task.");
    dev::CurrentThreadIds().RefreshLogic(0);
}


//-------------------------------------------------------------------------------------------------
yaget::Application::~Application()
{
}


//-------------------------------------------------------------------------------------------------
int yaget::Application::Run(const TickLogic& tickLogic, const TickRender& tickRender /*= {}*/, const TickIdle& tickIdle /*= {}*/, const QuitLogic& shutdownLogicCallback /*= {}*/, const QuitApplication& quitCallback /*= {}*/)
{
    // kick off separate thread for rendering
    YAGET_ASSERT(mGeneralPoolThread, "Can not call Run second time.");
    if (tickRender)
    {
        mGeneralPoolThread->AddTask([this, tickRender]() { onRenderTask(tickRender); });
    }
    YAGET_ASSERT(tickLogic, "Run Application method need to have valid TickLogic callback.");
    mGeneralPoolThread->AddTask([this, &tickLogic, shutdownLogicCallback]() { onLogicTask(tickLogic, shutdownLogicCallback); });

    mGeneralPoolThread->UnpauseAll();
    YLOG_DEBUG("APP", "Application.Run pump started.");

    constexpr time::Microsecond_t oneSecond = time::FromTo<time::Microsecond_t>(1.0f, time::kSecondUnit, time::kMicrosecondUnit);
    time::Microsecond_t printInterval = platform::GetRealTime(yaget::time::kMicrosecondUnit) + oneSecond;
    while (!mRequestQuit)
    {
        onMessagePump(mApplicationClock);
        if (tickIdle)
        {
            tickIdle();
        }

        const auto nowTime = platform::GetRealTime(yaget::time::kMicrosecondUnit);
        if (printInterval < nowTime)
        {
            printInterval = nowTime + oneSecond;

            const auto avgLogicDelta = time::FromTo<float>(mLogicFrameCounter.GetAvgDelta(), time::kMicrosecondUnit, time::kMilisecondUnit);
            const auto loopLogicDelta = time::FromTo<float>(mLogicFrameCounter.GetLoopDelta(), time::kMicrosecondUnit, time::kMilisecondUnit);

            const auto avgRenderDelta = time::FromTo<float>(mRenderFrameCounter.GetAvgDelta(), time::kMicrosecondUnit, time::kMilisecondUnit);
            const auto loopRenderDelta = time::FromTo<float>(mRenderFrameCounter.GetLoopDelta(), time::kMicrosecondUnit, time::kMilisecondUnit);

            const auto logicFPS = static_cast<int>(loopLogicDelta ? 1000/loopLogicDelta : 0);
            const auto renderFPS = static_cast<int>(loopRenderDelta ? 1000/loopRenderDelta : 0);
            YLOG_DEBUG("APP", "Logic Frame: %.3f ms., Logic Loop: %.3f ms. (%d), Render Frame: %.3f ms., Render Loop: %.3f ms. (%d)", avgLogicDelta, loopLogicDelta, logicFPS, avgRenderDelta, loopRenderDelta, renderFPS);
        }

        std::this_thread::yield();
    }
    YLOG_DEBUG("APP", "Application.Run pump ended.");

    if (quitCallback)
    {
        quitCallback();
    }

    // cleanup all threads here, wait for all completion before proceeding
    mQuit = true;
    mGeneralPoolThread.reset();
    YLOG_DEBUG("APP", "Application.Run mGeneralPoolThread stopped and cleared.");

    Cleanup();
    while (onMessagePump(mApplicationClock))
        ;

    YLOG_DEBUG("APP", "Application.Run returning back to caller.");
    return 0;
}


//-------------------------------------------------------------------------------------------------
void yaget::Application::RequestQuit()
{
    mRequestQuit = true;
}


//-------------------------------------------------------------------------------------------------
void yaget::Application::ChangeVideoSettings(const VideoOptions& /*videoOptions*/)
{
}

