#include "pch.h" 
#include "Streams/Guid.h"
#include "TestHelpers/TestHelpers.h"


TEST(YagetCore, Types_Guid)
{
    yaget::test::Environment environment;
    /*************************************************************************
    * HAPPY PATH TESTS
    *************************************************************************/

    auto r1 = yaget::NewGuid();
    auto r2 = yaget::NewGuid();
    auto r3 = yaget::NewGuid();

    yaget::Guid s1("7bcd757f-5b10-4f9b-af69-1a1f226f3b3e");
    yaget::Guid s2("16d1bd03-09a5-47d3-944b-5e326fd52d27");
    yaget::Guid s3("fdaba646-e07e-49de-9529-4499a5580c75");
    yaget::Guid s4("7bcd757f-5b10-4f9b-af69-1a1f226f3b3e");

    std::hash<yaget::Guid> hasher;
    const auto hashValue1 = hasher(s1);
    EXPECT_NE(hashValue1, 0);

    const auto hashValue2 = hasher(s2);
    EXPECT_NE(hashValue1, hashValue2);

    EXPECT_EQ(r1 != r2 || r1 != r3 || r2 != r3, true);
    EXPECT_NE(s1, s2);
    EXPECT_EQ(s1, s4);

    std::stringstream ss1;
    ss1 << s1;
    EXPECT_EQ(ss1.str(), "7bcd757f-5b10-4f9b-af69-1a1f226f3b3e");

    EXPECT_EQ(s1.str(), "7bcd757f-5b10-4f9b-af69-1a1f226f3b3e");

    std::stringstream ss2;
    ss2 << s2;
    EXPECT_EQ(ss2.str(), "16d1bd03-09a5-47d3-944b-5e326fd52d27");

    std::stringstream ss3;
    ss3 << s3;
    EXPECT_EQ(ss3.str(), "fdaba646-e07e-49de-9529-4499a5580c75");

    auto swap1 = yaget::NewGuid();
    auto swap2 = yaget::NewGuid();
    auto swap3 = swap1;
    auto swap4 = swap2;

    EXPECT_EQ(swap1, swap3);
    EXPECT_EQ(swap2, swap4);
    EXPECT_NE(swap1, swap2);

    using std::swap;
    swap(swap1, swap2);

    EXPECT_EQ(swap1, swap4);
    EXPECT_EQ(swap2, swap3);
    EXPECT_NE(swap1, swap2);

    std::array<unsigned char, 16> bytes =
    {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xdd
    };
    yaget::Guid guidFromBytes(bytes);
    yaget::Guid guidFromString("0102030405060708090a0b0c0d0e0fdd");
    EXPECT_EQ(guidFromBytes, guidFromString);

    EXPECT_EQ(std::equal(guidFromBytes.bytes().begin(), guidFromBytes.bytes().end(), bytes.begin()), true);

    /////*************************************************************************
    ////* ERROR HANDLING
    ////*************************************************************************/

    yaget::Guid empty;
    yaget::Guid twoTooFew("7bcd757f-5b10-4f9b-af69-1a1f226f3b");
    EXPECT_EQ(twoTooFew, empty);
    EXPECT_EQ(false, twoTooFew.IsValid());

    yaget::Guid oneTooFew("16d1bd03-09a5-47d3-944b-5e326fd52d2");
    EXPECT_EQ(oneTooFew, empty);
    EXPECT_EQ(false, twoTooFew.IsValid());

    yaget::Guid twoTooMany("7bcd757f-5b10-4f9b-af69-1a1f226f3beeff");
    EXPECT_EQ(twoTooMany, empty);
    EXPECT_EQ(false, twoTooMany.IsValid());

    yaget::Guid oneTooMany("16d1bd03-09a5-47d3-944b-5e326fd52d27a");
    EXPECT_EQ(oneTooMany, empty);
    EXPECT_EQ(false, oneTooMany.IsValid());

    yaget::Guid badString("!!bad-guid-string!!");
    EXPECT_EQ(badString, empty);
    EXPECT_EQ(false, badString.IsValid());
}