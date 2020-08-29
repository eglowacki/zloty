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


TEST(YagetCoreTest, JobPool)
{
    using namespace yaget;

    test::Environment environment;

    const int Iterations = 1000000;
    const int MaxThreads = 4;
    std::map<uint32_t, std::atomic_int> WorkLoads;

    const auto& message = fmt::format("Running '{}' tasks with '{}' threads", conv::ToThousandsSep(Iterations), MaxThreads);
    const auto& message2 = fmt::format("Adding '{}' tasks", conv::ToThousandsSep(Iterations));

    metrics::TimeScoper<time::kMilisecondUnit> cleanupTimer(message.c_str(), YAGET_LOG_FILE_LINE_FUNCTION);
    std::atomic_int counter{ Iterations };

    {
        mt::JobPool pool("UNIT_TEST", MaxThreads, mt::JobPool::Behaviour::StartAsPause);
        {
            metrics::TimeScoper<time::kMilisecondUnit> intTimer(message2.c_str(), YAGET_LOG_FILE_LINE_FUNCTION);

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

        metrics::TimeScoper<time::kMilisecondUnit> runThreadsTimer("Run all jobs", YAGET_LOG_FILE_LINE_FUNCTION);
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
}
