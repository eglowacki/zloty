#include "pch.h" 
#include "YagetCore.h"
#include "MemoryManager/PoolAllocator.h"
#include "MathFacade.h"
#include "Platform/Support.h"
#include "Logger/YLog.h"
#include "Exception/Exception.h"
#include "Metrics/Gather.h"
#include "TestHelpers/TestHelpers.h"

//#include "ImageLoaders/lodepng.h"

#include <functional>

class PoolAllocators : public ::testing::Test
{
protected:
    // void SetUp() override {}
    // void TearDown() override {}

private:
    //yaget::test::Environment mEnvironment;
};

namespace
{
    const int kTesterNumber = 8;
    const int kUserNumber = 14;

    struct TestClass
    {
        TestClass()
            : z(kTesterNumber)
        {}

        TestClass(int newValue)
            : z(newValue)
        {}

        ~TestClass()
        {
            z *= -1;
        }
        int z;
    };



} // namespace


TEST_F(PoolAllocators, Basic)
{
    using namespace yaget;

    memory::PoolAllocator<TestClass, 32> testPoolAllocator;

    // check if ctor and dtor runs on allocated object (TestClass)
    TestClass* testClass = testPoolAllocator.Allocate();
    EXPECT_TRUE(testClass);
    EXPECT_EQ(testClass->z, kTesterNumber);
    testPoolAllocator.Free(testClass);

    TestClass* smartTestClass = nullptr;
    EXPECT_EQ(testClass->z, -kTesterNumber);

    // check if ctor and dtor runs on allocated object (TestClass), but with shared_ptr
    {
        std::shared_ptr<TestClass> testClassHandle = memory::New(testPoolAllocator);
        EXPECT_TRUE(testClassHandle);
        EXPECT_EQ(testClassHandle->z, kTesterNumber);

        // since we really do not free the memory, we keep raw pointer so we can check outside this block that dtor was run on shared_ptr DO NOT DO THIS IN PRODUCTION CODE
        smartTestClass = testClassHandle.get();
    }
    EXPECT_EQ(smartTestClass->z, -kTesterNumber);

    // check if ctor and dtor runs on allocated object (TestClass) with ctor parameters
    testClass = testPoolAllocator.Allocate(kUserNumber);
    EXPECT_EQ(testClass->z, kUserNumber);
    testPoolAllocator.Free(testClass);
    EXPECT_EQ(testClass->z, -kUserNumber);
}

TEST_F(PoolAllocators, Capacity)
{
    using namespace yaget;
    using TC = std::shared_ptr<TestClass>;
    const int kNumberItems = 256;

    memory::PoolAllocator<TestClass, 32> testPoolAllocator;
    std::vector<TC> objectList;

    for (int i = 0; i < kNumberItems; ++i)
    {
        TC testClass = memory::New(testPoolAllocator, i);
        //--CHECK_EQUAL(testClass->z, i);
        objectList.push_back(testClass);
    }

    for (int i = 0; i < 100; ++i)
    {
        int index = rng::GetDice(kNumberItems);
        if (objectList[index])
        {
            TestClass* destroyedItem = objectList[index].get();
            objectList[index] = nullptr;
            //--CHECK_EQUAL(destroyedItem->z, -index);
        }
    }
}

TEST_F(PoolAllocators, Performance)
{
    using namespace yaget;
    using TC = std::shared_ptr<TestClass>;
    const int kNumberItems = 1024 * 5;

    memory::PoolAllocator<TestClass, 64> testPoolAllocator;
    {
        std::vector<TC> objectList;
        objectList.reserve(kNumberItems);

        time::Microsecond_t startTimeMicr = platform::GetRealTime(time::kMicrosecondUnit);

        for (int i = 0; i < kNumberItems; ++i)
        {
            TC testClass = memory::New(testPoolAllocator, i);
            objectList.push_back(testClass);
        }
        time::Microsecond_t endTimeMicr = platform::GetRealTime(time::kMicrosecondUnit);
        time::Microsecond_t totalTimeMicr = endTimeMicr - startTimeMicr;

#ifdef YAGET_RELEASE
        //--CHECK(totalTimeMicr < 2500);
#endif // YAGET_RELEASE

        YLOG_INFO("PROF", "PoolAllocator for '%d' items took '%d' microseconds.", kNumberItems, totalTimeMicr);
    }

    {
        std::vector<TC> objectList;
        objectList.reserve(kNumberItems);

        time::Microsecond_t startTimeMicr = platform::GetRealTime(time::kMicrosecondUnit);

        for (int i = 0; i < kNumberItems; ++i)
        {
            TC testClass = memory::New(testPoolAllocator, i);
            objectList.push_back(testClass);
        }
        time::Microsecond_t endTimeMicr = platform::GetRealTime(time::kMicrosecondUnit);
        time::Microsecond_t totalTimeMicr = endTimeMicr - startTimeMicr;

#ifdef YAGET_RELEASE
        //--CHECK(totalTimeMicr < 2500);
#endif // YAGET_RELEASE

        YLOG_INFO("PROF", "Pre-warmed PoolAllocator SmartPtr for '%d' items took '%d' microseconds.", kNumberItems, totalTimeMicr);
    }

    {
        YM_GATHER_RESET;

        std::vector<TestClass*> pointerList;
        pointerList.reserve(kNumberItems);

        time::Microsecond_t startTimeMicr = platform::GetRealTime(time::kMicrosecondUnit);

        for (int i = 0; i < kNumberItems; ++i)
        {
            TestClass* testClass = testPoolAllocator.Allocate(i);
            pointerList.push_back(testClass);
        }
        time::Microsecond_t endTimeMicr = platform::GetRealTime(time::kMicrosecondUnit);
        time::Microsecond_t totalTimeMicr = endTimeMicr - startTimeMicr;
        YLOG_INFO("PROF", "Pre-warmed PoolAllocator Raw for '%d' items took '%d' microseconds.", kNumberItems, totalTimeMicr);

        std::for_each(pointerList.begin(), pointerList.end(), [&testPoolAllocator](TestClass* element)
        {
            testPoolAllocator.Free(element);
        });

        const metrics::Gather& gather = metrics::Gather::Get();

        for (auto it : gather.GetResults())
        {
            const metrics::Gather::MarkerData& marker = it.second;

            time::Microsecond_t timeSpend = time::FromTo<time::Microsecond_t>(marker.mTime, time::kRawUnit, time::kMicrosecondUnit);
            YLOG_INFO("PROF", "=== Section %s: '%d' microseconds.", marker.mName.c_str(), timeSpend);

            //template <typename T, typename V>
            //T FromTo(V value, TimeUnits_t from, TimeUnits_t to)
        }
    }


    std::vector<TestClass*> pointerList;
    pointerList.reserve(kNumberItems);

    time::Microsecond_t startTimeMicr = platform::GetRealTime(time::kMicrosecondUnit);

    for (int i = 0; i < kNumberItems; ++i)
    {
        TestClass* testClass = new TestClass(i);
        pointerList.push_back(testClass);
    }
    time::Microsecond_t endTimeMicr = platform::GetRealTime(time::kMicrosecondUnit);
    time::Microsecond_t totalTimeMicr = endTimeMicr - startTimeMicr;
    YLOG_INFO("PROF", "new for '%d' items took '%d' microseconds.", kNumberItems, totalTimeMicr);
}

TEST_F(PoolAllocators, Iterators)
{
    using namespace yaget;
    constexpr int kPoolLineSize = 64;
    constexpr int kPoolSize = 64;
    constexpr const int kNumberItems = kPoolLineSize * kPoolSize;

    using PoolAllocator = memory::PoolAllocator<TestClass, kPoolLineSize>;
    PoolAllocator testPoolAllocator;

    auto allocateCounter = 0;
    for (int i = 0; i < kPoolSize; i += 2)
    {
        for (int j = 0; j < kPoolLineSize; ++j)
        {
            auto* newObject = testPoolAllocator.Allocate(allocateCounter++);

            //TC testClass = memory::New(testPoolAllocator, i);
            //objectList.push_back(testClass);
            int z = 0;
            z;
        }
    }

    const std::string filename = util::ExpendEnv("$(LogFolder)/PoolAllocImage", "png");

    std::vector<image::pixel_byte> bytes;
    bytes.resize(kPoolLineSize * kPoolSize);
    std::fill(bytes.begin(), bytes.end(), 0);

    int itemCounter = 0;

    for (auto it = testPoolAllocator.begin(); it != testPoolAllocator.end(); ++it)
    {
        auto object = &(*it);
        
        auto* blockHeader = PoolAllocator::PoolLine::GetBlockHeader(object);
        const auto cellNumber = blockHeader->mLineIndex * kPoolSize + blockHeader->mSlotIndex;

        EXPECT_TRUE(object->z == cellNumber);

        bytes[object->z] = 255;

        ++itemCounter;
    }

    const auto result = image::EncodeSave(filename, bytes, kPoolLineSize, kPoolSize, 0/*LodePNGColorType LCT_GREY*/);

    itemCounter = 0;
    for (auto it = testPoolAllocator.begin(); it != testPoolAllocator.end(); ++it)
    {
        auto* object = &(*it);

        testPoolAllocator.Free(object);
        EXPECT_TRUE(object->z == -itemCounter);

        ++itemCounter;
    }                                   

    int z = 0;
    z;
}
