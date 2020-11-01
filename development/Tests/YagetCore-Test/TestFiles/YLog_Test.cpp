#include "pch.h" 
#include "TestHelpers/TestHelpers.h"


class YLog : public ::testing::Test
{
   
};


TEST_F(YLog, Tagger)
{
    using namespace yaget;

    // any new changes run each individual line uncommented to ensure that asserts (so far, we do not have an easy way to control yaget asserts from crashing)
    //const char* Tag_Null = nullptr;
    //const char* Tag_Empty = "";
    //ylog::Tagger tn(Tag_Empty);
    //ylog::Tagger tn(Tag_Empty);

    const char* Tag_One = "F";
    const char* Tag_Two = "ON";
    const char* Tag_Three = "FOO";
    const char* Tag_Four = "VOID";
    const char* Tag_TooLong = "VOIDVERBOSE";

    const char* Tag_Mulform = "VO\0D";
    const char* Tag_MulformMatch = "VO";


    ylog::Tagger t(Tag_One);
    EXPECT_STREQ(Tag_One, t.c_str());
    uint32_t number = t;

    ylog::Tagger n(number);
    EXPECT_STREQ(Tag_One, n.c_str());

    //
    ylog::Tagger t2(Tag_Two);
    EXPECT_STREQ(Tag_Two, t2.c_str());
    uint32_t number2 = t2;

    ylog::Tagger n2(number2);
    EXPECT_STREQ(Tag_Two, n2.c_str());

    //
    ylog::Tagger t3(Tag_Three);
    EXPECT_STREQ(Tag_Three, t3.c_str());
    uint32_t number3 = t3;

    ylog::Tagger n3(number3);
    EXPECT_STREQ(Tag_Three, n3.c_str());

    //
    ylog::Tagger t4(Tag_Four);
    EXPECT_STREQ(Tag_Four, t4.c_str());
    uint32_t number4 = t4;

    ylog::Tagger n4(number4);
    EXPECT_STREQ(Tag_Four, n4.c_str());

    //
    ylog::Tagger t5(Tag_TooLong);
    EXPECT_STREQ(Tag_Four, t5.c_str());
    uint32_t number5 = t5;
    EXPECT_EQ(number4, number5);

    ylog::Tagger n5(number5);
    EXPECT_STREQ(Tag_Four, n5.c_str());

    //
    ylog::Tagger t6(Tag_Mulform);
    EXPECT_STREQ(Tag_MulformMatch, t6.c_str());
    uint32_t number6 = t6;

    ylog::Tagger n6(number6);
    EXPECT_STREQ(Tag_MulformMatch, n6.c_str());
}

