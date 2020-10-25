#include "pch.h" 
#include "Meta/CompilerAlgo.h"
#include "TestHelpers/TestHelpers.h"

namespace
{
    const char* foo = "Foo";
    const char* bar = "Bar";
    const char* empty = "EMPTY";


    struct TupleCopyPolicy
    {
        template <typename TS, typename TT>
        static constexpr TS Copy(TS source, TT target)
        {
            return source ? source : target;
        }

        template <>
        static constexpr const char* Copy(const char* source, const char* target)
        {
            return source ? source : empty;
        }

        template <>
        static constexpr int Copy(int source, int target)
        {
            return source ? source : 417;
        }
    };
}


class CompilerAlgo : public ::testing::Test
{
};



TEST_F(CompilerAlgo, TupleCopy)
{
    // test values and convenience using
    using namespace yaget;
    using Target = std::tuple<int, bool, float, const char*>;
    using SourceOne = std::tuple<int, bool>;
    using SourceTwo = std::tuple<float, const char*>;
    using SourceThree = Target;

    // test copy form source to target using different parts and finally testing not overriding value if 'false'
    Target target                   {3, false, 4.17f, foo };
    const SourceOne sourceOne       {2, true };
    const Target expectedTargetOne  {2, true,  4.17f, foo };

    const SourceTwo sourceTwo       {           2.0f, bar };
    const Target expectedTargetTwo  { 2, true,  2.0f, bar };
    // this will not change any target values
    const SourceThree sourceThree{ 0, false, 0.0f, nullptr };

    auto originalTarget = target;
    meta::tuple_copy(sourceOne, target);
    EXPECT_EQ(target, expectedTargetOne);

    meta::tuple_copy(sourceTwo, target);
    EXPECT_EQ(target, expectedTargetTwo);

    meta::tuple_copy(sourceThree, target);
    EXPECT_EQ(target, expectedTargetTwo);

    meta::tuple_copy(sourceThree, target);
    EXPECT_EQ(target, expectedTargetTwo);

    // test that we can customize how element type is copied, for empty char* we'll get "EMPTY"
    // and for int 0, we should get 417
    const Target expectedTargetThree{ 417, true,  2.0f, empty };
    meta::tuple_copy<SourceThree, Target, TupleCopyPolicy>(sourceThree, target);
    EXPECT_EQ(target, expectedTargetThree);
}

TEST_F(CompilerAlgo, TupleBits)
{
    using namespace yaget;
    const meta::bits_t expectedLocationBits = 0b10100;
    using Location = std::tuple<int, bool>;
    using Tree = std::tuple<float, const char*, bool, double, int>;
    auto locationBits = meta::tuple_bit_pattern_v<Tree, Location>;
    EXPECT_EQ(locationBits, expectedLocationBits);

    const meta::bits_t expectedFullBits = 0b11111;
    using FullRow = std::tuple<float, const char*, bool, double, int>;
    auto fullBits = meta::tuple_bit_pattern_v<Tree, FullRow>;
    EXPECT_EQ(fullBits, expectedFullBits);

    auto sameBits = meta::tuple_bit_pattern_v<Tree>;
    EXPECT_EQ(sameBits, expectedFullBits);
}

TEST_F(CompilerAlgo, TupleCombine)
{
    using namespace yaget;
    using Dummy = std::tuple<>;
    using SourceOne = std::tuple<int, bool, float, char>;
    using SourceTwo = std::tuple<int, char>;
    using SourceThree = std::tuple<float, const char, bool>;

    using TestOne = std::tuple<int, bool, float, char>;
    using TestOneTwo = std::tuple<int, bool, float, char, int, char>;
    using TestOneTwoThree = std::tuple<int, bool, float, char, int, char, float, const char, bool>;
    using TestOneDummyTwoThree = std::tuple<int, bool, float, char, int, char, float, const char, bool>;
    using DummyTestOneTwoThree = std::tuple<int, bool, float, char, int, char, float, const char, bool>;
    using TestOneTwoThreeDummy = std::tuple<int, bool, float, char, int, char, float, const char, bool>;

    using Row1 = meta::tuple_combine_t<SourceOne>;
    EXPECT_EQ(typeid(Row1), typeid(TestOne));

    using Row12 = meta::tuple_combine_t<SourceOne, SourceTwo>;
    EXPECT_EQ(typeid(Row12), typeid(TestOneTwo));
    EXPECT_NE(typeid(Row12), typeid(TestOne));

    using Row123 = meta::tuple_combine_t<SourceOne, SourceTwo, SourceThree>;
    EXPECT_EQ(typeid(Row123), typeid(TestOneTwoThree));

    using Row1d23 = meta::tuple_combine_t<SourceOne, Dummy, SourceTwo, SourceThree>;
    EXPECT_EQ(typeid(Row1d23), typeid(TestOneDummyTwoThree));

    using Rowd123 = meta::tuple_combine_t<Dummy, SourceOne, SourceTwo, SourceThree>;
    EXPECT_EQ(typeid(Rowd123), typeid(DummyTestOneTwoThree));

    using Row123d = meta::tuple_combine_t<SourceOne, SourceTwo, SourceThree, Dummy>;
    EXPECT_EQ(typeid(Row123d), typeid(TestOneTwoThreeDummy));
}


