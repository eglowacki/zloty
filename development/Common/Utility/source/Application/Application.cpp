#include "App/Application.h"
#include "Debugging/DevConfiguration.h"


using namespace yaget;

Application::Application(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options)
    : mDirector(director)
    , Options(options)
    , IdCache(&mDirector)
    , mInputDevice(vts)
    , mGeneralPoolThread(std::make_unique<mt::JobPool>("AppPool", yaget::dev::CurrentConfiguration().mDebug.mThreads.App))
    , mVTS(vts)
{
    YLOG_INFO("INIT", "Created Application '%s'.", title.c_str());
}

Application::~Application()
{
}

void Application::onRenderTask(UpdateCallback_t renderCallback)
{
    dev::CurrentThreadIds().RefreshRender(platform::CurrentThreadId());
    metrics::MarkStartThread(dev::CurrentThreadIds().Render, "Rendering");

    metrics::Channel channel("RenderThread", YAGET_METRICS_CHANNEL_FILE_LINE);

    while (!mQuit)
    {
        metrics::Channel channel("RenderTick", YAGET_METRICS_CHANNEL_FILE_LINE);

        renderCallback(*this, mGameClock, channel);
        std::this_thread::yield();
    }

    dev::CurrentThreadIds().RefreshRender(0);
}

void Application::onLogicTask(UpdateCallback_t logicCallback)
{
    dev::CurrentThreadIds().RefreshLogic(platform::CurrentThreadId());
    metrics::MarkStartThread(dev::CurrentThreadIds().Logic, "Game Logic");


    metrics::Channel channel("GameThread", YAGET_METRICS_CHANNEL_FILE_LINE);

    const time::Microsecond_t kFixedDeltaTime = time::GetDeltaTime(dev::CurrentConfiguration().mInit.LogicTick);
    metrics::PerformancePolicy defaultPerformancePolicy;

    mGameClock.Resync();
    time::Microsecond_t startTime = platform::GetRealTime(time::kMicrosecondUnit);
    time::Microsecond_t currentTickTime = startTime;
    time::Microsecond_t tickAccumulator = 0;

    using State = double;

    State previousState(0);
    State currentState(0);

    do
    {
        time::Microsecond_t nextTickTime = platform::GetRealTime(time::kMicrosecondUnit);
        time::Microsecond_t deltaTickTime = nextTickTime - currentTickTime;
        tickAccumulator += deltaTickTime;

        while (tickAccumulator >= kFixedDeltaTime)
        {
            metrics::Channel channel("GameTick", YAGET_METRICS_CHANNEL_FILE_LINE);

            time::Microsecond_t startProccessTime = platform::GetRealTime(time::kMicrosecondUnit);
            const uint64_t tickCounter = mGameClock.GetTickCounter();

            mGameClock.Tick(kFixedDeltaTime);
            /*uint32_t numInputs =*/ mInputDevice.Tick(mGameClock, defaultPerformancePolicy, channel);

            tickAccumulator -= kFixedDeltaTime;

            if (logicCallback)
            {
                metrics::Channel channel("Callback", YAGET_METRICS_CHANNEL_FILE_LINE);

                logicCallback(*this, mGameClock, channel);
            }

            time::Microsecond_t actualProccessTime = platform::GetRealTime(time::kMicrosecondUnit) - startProccessTime;
            if (actualProccessTime > kFixedDeltaTime)
            {
                YLOG_DEBUG("PROF", "Tick Loop tool too long. Budget: '%d' (mc), Actual: '%d' (mc).", kFixedDeltaTime, actualProccessTime);
                if (platform::IsDebuggerAttached())
                {
                    // since we are under debugger, we just adjust the main timer to account for that loss time (maybe due to break point)
                    platform::AdjustDrift(actualProccessTime - kFixedDeltaTime, time::kMicrosecondUnit);
                    YLOG_DEBUG("PROF", "Adjusted Main Real Time by: '%d' (mc).", actualProccessTime - kFixedDeltaTime);
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

    dev::CurrentThreadIds().RefreshLogic(0);
}

int Application::Run(UpdateCallback_t logicCallback, UpdateCallback_t renderCallback, StatusCallback_t idleCallback, StatusCallback_t quitCallback)
{
    // kick off separate thread for rendering
    YAGET_ASSERT(mGeneralPoolThread, "Can not call Run second time.");
    if (renderCallback)
    {
        mGeneralPoolThread->AddTask([this, renderCallback]() { onRenderTask(renderCallback); });
    }
    mGeneralPoolThread->AddTask([this, logicCallback]() { onLogicTask(logicCallback); });

    YLOG_DEBUG("APP", "Application.Run pump started.");
    while (!mRequestQuit)
    {
        //metrics::Channel channel("Message Pump", YAGET_METRICS_CHANNEL_FILE_LINE);

        onMessagePump(mGameClock);
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
    while (onMessagePump(mGameClock))
        ;

    YLOG_DEBUG("APP", "Application.Run returning back to caller!");
    return 0;
}

void Application::RequestQuit()
{
    mRequestQuit = true;
}

void Application::AddTask(const mt::JobProcessor::Task_t& task)
{
    if (mGeneralPoolThread)
    {
        mGeneralPoolThread->AddTask(task);
    }
}

void yaget::Application::ChangeVideoSettings(const VideoOptions& /*videoOptions*/)
{
}
