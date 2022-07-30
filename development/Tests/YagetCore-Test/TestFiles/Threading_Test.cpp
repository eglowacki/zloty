#include "pch.h" 

#include "App/AppHarness.h"
#include "ThreadModel/JobPool.h"
#include "ThreadModel/Variables.h"
#include "TestHelpers/TestHelpers.h"


//CHECK_EQUAL(expected, actual);
class Threading : public ::testing::Test
{
};


class Foo : public yaget::NonCopyMove<Foo>
{
private:
    int mZ = 417;
};

TEST_F(Threading, Condition)
{
    using namespace yaget;

#if 0
    Foo foo;
    Foo bar;
    bar = foo;
#endif

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


namespace
{
    struct ProtectedData
    {
        std::string mName;
        double mLargeNumber = 0;
    };

    enum class StateOperation : uint32_t
    {
        Idle,
        ReadValue,
        GotValue
    };
}


TEST_F(Threading, Variable)
{
    using namespace yaget;
    using VProtectedData = mt::Variable<ProtectedData>;

    mt::JobPool pool("VARIABLE_TEST", 1);

    const std::string testName = "Racing with 417";
    const double testNumber = 3.123456789;
    VProtectedData protectedData{testName, testNumber};

    bool quit = false;
    StateOperation stateOperation = StateOperation::Idle;

    auto checkValueForRead = [&quit, &stateOperation, &protectedData, testNumber, testName]()
    {
        while (!quit)
        {
            if (stateOperation == StateOperation::ReadValue)
            {
                ProtectedData v;
                if (protectedData.Get(v))
                {
                    EXPECT_EQ(testName, v.mName);
                    EXPECT_EQ(testNumber + 10, v.mLargeNumber);

                    stateOperation = StateOperation::GotValue;
                    break;
                }
            }

            std::this_thread::yield();
        }
    };

    pool.AddTask(checkValueForRead);

    ProtectedData data1 = protectedData;
    EXPECT_EQ(testName, data1.mName);
    EXPECT_EQ(testNumber, data1.mLargeNumber);

    {
        auto locker = protectedData.GetLocker();
        locker.mDataValue.mLargeNumber += 10;

        stateOperation = StateOperation::ReadValue;
        platform::Sleep(50, time::kMilisecondUnit);
    }

    platform::Sleep(50, time::kMilisecondUnit);

    EXPECT_EQ(StateOperation::GotValue, stateOperation);
    EXPECT_EQ(testNumber, data1.mLargeNumber);

    ProtectedData data2 = protectedData;
    EXPECT_EQ(testNumber + 10, data2.mLargeNumber);

    quit = true;
}
