#include "pch.h" 
#include "ThreadModel/JobPool.h" 
#include "fmt/format.h" 
#include "Metrics/Gather.h" 

namespace yaget::ylog
{
    yaget::Strings GetRegisteredTags()
    {
        yaget::Strings tags =
        {
            #include "Logger/LogTags.h" 
        };

        return tags;
    }
} // namespace yaget::ylog 

TEST(YagetCoreTest, JobPool)
{
    using namespace yaget;

    const int Iterations = 1000000;
    const int MaxThreads = 1;

    const auto& message = fmt::format("Running '{}' tasks with '{}' threads", conv::ToThousandsSep(Iterations), MaxThreads);
    const auto& message2 = fmt::format("Adding '{}' tasks", conv::ToThousandsSep(Iterations));

    metrics::TimeScoper<time::kMilisecondUnit> cleanupTimer(message.c_str(), YAGET_LOG_FILE_LINE_FUNCTION);
    std::atomic_int counter{ Iterations };

    {
        mt::JobPool pool("unit_test", MaxThreads, mt::JobPool::Behaviour::StartAsPause);

        {
            metrics::TimeScoper<time::kMilisecondUnit> intTimer(message2.c_str(), YAGET_LOG_FILE_LINE_FUNCTION);

            auto f = [&counter]()
            {
                --counter;
            };

            for (int i = 0; i < Iterations; ++i)
            {
                pool.AddTask(f);
            }
        }

        pool.UnpauseAll();
        pool.Join();
    }

    EXPECT_EQ(counter, 0);
}
