#include "StringHelpers.h"
//#include "App/FileUtilities.h"
#include "VTS/VirtualTransportSystem.h"
#include "UnitTest++.h"

TEST(WildCardMatch)
{
    std::string fileName = "c:/foldera/folderb/bar.txt";

    CHECK(yaget::WildCompare("*.txt", fileName));
    CHECK(yaget::WildCompare("*.TXt", fileName));
    CHECK(!yaget::WildCompare("*.txy", fileName));
    CHECK(yaget::WildCompare("*.t?t", fileName));
    CHECK(yaget::WildCompare("*.T?t", fileName));
    CHECK(!yaget::WildCompare("*.t?y", fileName));
    CHECK(!yaget::WildCompare("*.txt", ""));
    CHECK(!yaget::WildCompare("", fileName));
    CHECK(!yaget::WildCompare("", ""));
}

TEST(VectorStringConversion)
{
    // check empty vector with any delimiters
    std::vector<std::string> testValues;
    std::string result;

    result = yaget::conv::Combine(testValues, nullptr);
    CHECK_EQUAL("", result);
    result = yaget::conv::Combine(testValues, "");
    CHECK_EQUAL("", result);
    result = yaget::conv::Combine(testValues, " ");
    CHECK_EQUAL("", result);
    result = yaget::conv::Combine(testValues, ",");
    CHECK_EQUAL("", result);


    // check for one element
    testValues = { "A" };

    result = yaget::conv::Combine(testValues, nullptr);
    CHECK_EQUAL("A", result);

    result = yaget::conv::Combine(testValues, "");
    CHECK_EQUAL("A", result);

    result = yaget::conv::Combine(testValues, " ");
    CHECK_EQUAL("A", result);

    result = yaget::conv::Combine(testValues, ",");
    CHECK_EQUAL("A", result);

    // check for one+ elements
    testValues = { "A", "B" };

    result = yaget::conv::Combine(testValues, nullptr);
    CHECK_EQUAL("AB", result);

    result = yaget::conv::Combine(testValues, "");
    CHECK_EQUAL("AB", result);

    result = yaget::conv::Combine(testValues, " ");
    CHECK_EQUAL("A B", result);

    result = yaget::conv::Combine(testValues, ",");
    CHECK_EQUAL("A,B", result);
}

TEST(StringVectorConversion)
{
    // emppty vector
    std::vector<std::string> result;
    std::string testValues;

    // test empty string
    testValues = "";

    result = yaget::conv::Split(testValues, "");
    CHECK(result == std::vector<std::string>());

    result = yaget::conv::Split(testValues, ",");
    CHECK(result == std::vector<std::string>());

    // one element on vector
    testValues = "A";

    result = yaget::conv::Split(testValues, "");
    CHECK(result == std::vector<std::string>{"A"});

    result = yaget::conv::Split(testValues, ",");
    CHECK(result == std::vector<std::string>{"A"});

    // two element on vector
    testValues = "A,B";

   result = yaget::conv::Split(testValues, "");
   CHECK(result == std::vector<std::string>{"A,B"});

   result = yaget::conv::Split(testValues, ",");
   auto cv = std::vector<std::string>{"A", "B"};
   CHECK(result == cv);

   result = yaget::conv::Split(testValues, ", ");
   CHECK(result == std::vector<std::string>{"A,B"});
}

TEST(StringNumberConversion)
{
    int64_t result = yaget::conv::Atoll("");
    CHECK_EQUAL(0, result);
    result = yaget::conv::Atoll(nullptr);
    CHECK_EQUAL(0, result);
    result = yaget::conv::Atoll("f");
    CHECK_EQUAL(0, result);
    result = yaget::conv::Atoll("123");
    CHECK_EQUAL(123, result);
    result = yaget::conv::Atoll("-123");
    CHECK_EQUAL(-123, result);
}

TEST(StringFilePath)
{
    std::string testPath = "/foo/";
    testPath = yaget::io::NormalizePath(testPath);
    CHECK_EQUAL("/foo/", testPath);

    testPath = "\\/\\\\foo\\\\";
    testPath = yaget::io::NormalizePath(testPath);
    CHECK_EQUAL("/foo/", testPath);

    testPath = "TestSource@Attach/foo.txt";
    testPath = yaget::io::NormalizePath(testPath);
    CHECK_EQUAL("TestSource@Attach/foo.txt", testPath);
    
    testPath = "TestSource@Attach\\foo.txt";
    testPath = yaget::io::NormalizePath(testPath);
    CHECK_EQUAL("TestSource@Attach/foo.txt", testPath);
}

