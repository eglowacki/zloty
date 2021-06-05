#include "pch.h" 
#include "ThreadModel/JobPool.h" 
#include "fmt/format.h" 

#include "LoggerCpp/OutputDebug.h"
#include "LoggerCpp/OutputFile.h"


#include "Metrics/Gather.h" 
#include "Metrics/Concurrency.h"

#include "TestHelpers/TestHelpers.h"

namespace yaget::ylog
{
    yaget::Strings GetRegisteredTags()
    {
        yaget::Strings tags =
        {
            #include "Logger/LogTags.h"
            "TEST"
        };

        return tags;
    }
} // namespace yaget::ylog 


//CHECK_EQUAL(expected, actual);
class Threads : public ::testing::Test
{
private:
    yaget::test::Environment mEnvironment;
};


TEST_F(Threads, JobPool)
{
    using namespace yaget;

    const int Iterations = 10000;
    const int MaxThreads = 4;
    std::map<uint32_t, std::atomic_int> WorkLoads;

    const auto& message = fmt::format("Running '{}' tasks with '{}' threads", conv::ToThousandsSep(Iterations), MaxThreads);
    const auto& message2 = fmt::format("Adding '{}' tasks", conv::ToThousandsSep(Iterations));

    metrics::TimeScoper<time::kMilisecondUnit> cleanupTimer("TEST", message.c_str(), YAGET_LOG_FILE_LINE_FUNCTION);
    std::atomic_int counter{ Iterations };

    {
        mt::JobPool pool("UNIT_TEST", MaxThreads, mt::JobPool::Behaviour::StartAsPause);
        {
            metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", message2.c_str(), YAGET_LOG_FILE_LINE_FUNCTION);

            auto f = [&counter, &WorkLoads]()
            {
                --counter;
                ++WorkLoads[platform::CurrentThreadId()];
                //platform::BusySleep(500, time::kMicrosecondUnit);
            };

            std::vector functions(Iterations, f);

            auto locker = pool.GetLocker();
            locker.AddTasks(functions);
        }

        metrics::TimeScoper<time::kMilisecondUnit> runThreadsTimer("TEST", "Run all jobs", YAGET_LOG_FILE_LINE_FUNCTION);
        pool.UnpauseAll();
        pool.Join();
    }

    std::string loadsMessage = fmt::format("{} tasks processed, threads load:", conv::ToThousandsSep(Iterations));
    for (const auto& elem : WorkLoads)
    {
        loadsMessage += fmt::format("\n\tThreadId: {} = {}", metrics::MarkGetThreadName(elem.first), conv::ToThousandsSep(elem.second.load()));
    }
    YLOG_NOTICE("TEST", loadsMessage.c_str());
    EXPECT_EQ(counter, 0);

    {
        mt::JobPool pool("UNIT_TEST_STARTUP", MaxThreads, mt::JobPool::Behaviour::StartAsRun);

        std::atomic_bool keepRunning = true;
        int counter[2] = {};
        pool.AddTask([&keepRunning, &counter]()
        {
            while (keepRunning)
            {
                counter[0] = 1;
            }
        });

        pool.AddTask([&keepRunning, &counter]()
        {
                while (keepRunning)
                {
                    counter[1] = 1;
                }
        });

        const auto sleepResult = platform::Sleep(1000, time::kMicrosecondUnit, [&counter]()
        {
            if (counter[0] == 1 && counter[1] == 1)
            {
                return false;
            }

            return true;
        });

        EXPECT_EQ(platform::SleepResult::OK, sleepResult);
        keepRunning = false;
    }
}


TEST_F(Threads, AsyncWait)
{
    using namespace yaget;

    mt::JobPool job("AsyncWait", 1);

    job.Join();

    for (int i = 0; i < 2; ++i)
    {
        job.AddTask([]()
        {
            platform::BusySleep(100, time::kMilisecondUnit);
        });

        YLOG_NOTICE("TEST", "Should see this right away.");

        job.Join();
        YLOG_NOTICE("TEST", "About 100 miliseconds later.");

        job.Join();
        job.Join();
        job.Join();
        job.Join();
        job.Join();
    }
}

