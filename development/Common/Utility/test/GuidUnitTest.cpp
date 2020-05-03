#include "Streams/Guid.h"
#include "UnitTest++.h"

TEST(Guid)
{
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

    CHECK(r1 != r2 || r1 != r3 || r2 != r3);
    CHECK(s1 != s2);
    CHECK_EQUAL(s1, s4);

    std::stringstream ss1;
    ss1 << s1;
    CHECK_EQUAL(ss1.str(), "7bcd757f-5b10-4f9b-af69-1a1f226f3b3e");

    CHECK_EQUAL(s1.str(), "7bcd757f-5b10-4f9b-af69-1a1f226f3b3e");

    std::stringstream ss2;
    ss2 << s2;
    CHECK_EQUAL(ss2.str(), "16d1bd03-09a5-47d3-944b-5e326fd52d27");

    std::stringstream ss3;
    ss3 << s3;
    CHECK_EQUAL(ss3.str(), "fdaba646-e07e-49de-9529-4499a5580c75");

    auto swap1 = yaget::NewGuid();
    auto swap2 = yaget::NewGuid();
    auto swap3 = swap1;
    auto swap4 = swap2;

    CHECK_EQUAL(swap1, swap3);
    CHECK_EQUAL(swap2, swap4);
    CHECK(swap1 != swap2);

    swap1.swap(swap2);

    CHECK_EQUAL(swap1, swap4);
    CHECK_EQUAL(swap2, swap3);
    CHECK(swap1 != swap2);

    std::array<unsigned char, 16> bytes =
    {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xdd
    };
    yaget::Guid guidFromBytes(bytes);
    yaget::Guid guidFromString("0102030405060708090a0b0c0d0e0fdd");
    CHECK_EQUAL(guidFromBytes, guidFromString);

    CHECK(std::equal(guidFromBytes.bytes().begin(), guidFromBytes.bytes().end(), bytes.begin()));

    /////*************************************************************************
    ////* ERROR HANDLING
    ////*************************************************************************/

    yaget::Guid empty;
    yaget::Guid twoTooFew("7bcd757f-5b10-4f9b-af69-1a1f226f3b");
    CHECK_EQUAL(twoTooFew, empty);
    CHECK_EQUAL(false, twoTooFew.IsValid());

    yaget::Guid oneTooFew("16d1bd03-09a5-47d3-944b-5e326fd52d2");
    CHECK_EQUAL(oneTooFew, empty);
    CHECK_EQUAL(false, twoTooFew.IsValid());

    yaget::Guid twoTooMany("7bcd757f-5b10-4f9b-af69-1a1f226f3beeff");
    CHECK_EQUAL(twoTooMany, empty);
    CHECK_EQUAL(false, twoTooMany.IsValid());

    yaget::Guid oneTooMany("16d1bd03-09a5-47d3-944b-5e326fd52d27a");
    CHECK_EQUAL(oneTooMany, empty);
    CHECK_EQUAL(false, oneTooMany.IsValid());

    yaget::Guid badString("!!bad-guid-string!!");
    CHECK_EQUAL(badString, empty);
    CHECK_EQUAL(false, badString.IsValid());
}