#include "pch.h" 

#include "App/AppHarness.h"
#include "ThreadModel/JobPool.h"
#include "TestHelpers/TestHelpers.h"


//CHECK_EQUAL(expected, actual);
class Threading : public ::testing::Test
{
};


TEST_F(Threading, Condition)
{
    using namespace yaget;

    mt::JobPool pool("CONDITION_TEST", 1);

    mt::Condition condition;
    bool conditionTriggered = false;
    bool quit = false;

    pool.AddTask([&condition, &conditionTriggered, &quit]()
    {
        while (!quit)
        {
            condition.Wait();
            conditionTriggered = true;
        }
    });

    platform::Sleep(1, time::kMilisecondUnit);
    EXPECT_EQ(false, conditionTriggered);

    condition.Trigger();
    platform::Sleep(10, time::kMicrosecondUnit);
    EXPECT_EQ(true, conditionTriggered);
    quit = true;
    condition.Trigger();
    conditionTriggered = false;

    bool quit2 = false;
    int counter = 0;
    pool.AddTask([&condition, &counter, &quit2]()
    {
        while (!quit2)
        {
            condition.Wait(400, time::kMilisecondUnit);
            ++counter;
        }
    });

    platform::Sleep(1000, time::kMilisecondUnit);
    EXPECT_EQ(2, counter);
    quit2 = true;
    condition.Trigger();
}
