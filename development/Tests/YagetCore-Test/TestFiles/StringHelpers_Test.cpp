#include "pch.h" 
#include "StringHelpers.h"
#include "TestHelpers/TestHelpers.h"
#include "VTS/VirtualTransportSystem.h"


class Strings : public ::testing::Test
{
protected:
    // void SetUp() override {}
    // void TearDown() override {}

private:
    //yaget::test::Environment mEnvironment;
};

TEST_F(Strings, WildCardMatch)
{
    std::string fileName = "c:/foldera/folderb/bar.txt";

    EXPECT_TRUE(yaget::WildCompare("*.txt", fileName));
    EXPECT_TRUE(yaget::WildCompare("*.TXt", fileName));
    EXPECT_TRUE(!yaget::WildCompare("*.txy", fileName));
    EXPECT_TRUE(yaget::WildCompare("*.t?t", fileName));
    EXPECT_TRUE(yaget::WildCompare("*.T?t", fileName));
    EXPECT_TRUE(!yaget::WildCompare("*.t?y", fileName));
    EXPECT_TRUE(!yaget::WildCompare("*.txt", ""));
    EXPECT_TRUE(!yaget::WildCompare("", fileName));
    EXPECT_TRUE(!yaget::WildCompare("", ""));
}

TEST_F(Strings, FromVectorConversion)
{
    // check empty vector with any delimiters
    std::vector<std::string> testValues;
    std::string result;

    result = yaget::conv::Combine(testValues, nullptr);
    EXPECT_EQ("", result);
    result = yaget::conv::Combine(testValues, "");
    EXPECT_EQ("", result);
    result = yaget::conv::Combine(testValues, " ");
    EXPECT_EQ("", result);
    result = yaget::conv::Combine(testValues, ",");
    EXPECT_EQ("", result);


    // check for one element
    testValues = { "A" };

    result = yaget::conv::Combine(testValues, nullptr);
    EXPECT_EQ("A", result);

    result = yaget::conv::Combine(testValues, "");
    EXPECT_EQ("A", result);

    result = yaget::conv::Combine(testValues, " ");
    EXPECT_EQ("A", result);

    result = yaget::conv::Combine(testValues, ",");
    EXPECT_EQ("A", result);

    // check for one+ elements
    testValues = { "A", "B" };

    result = yaget::conv::Combine(testValues, nullptr);
    EXPECT_EQ("AB", result);

    result = yaget::conv::Combine(testValues, "");
    EXPECT_EQ("AB", result);

    result = yaget::conv::Combine(testValues, " ");
    EXPECT_EQ("A B", result);

    result = yaget::conv::Combine(testValues, ",");
    EXPECT_EQ("A,B", result);
}

TEST_F(Strings, ToVectorConversion)
{
    // emppty vector
    std::vector<std::string> result;
    std::string testValues;

    // test empty string
    testValues = "";

    result = yaget::conv::Split(testValues, "");
    EXPECT_TRUE(result == std::vector<std::string>());

    result = yaget::conv::Split(testValues, ",");
    EXPECT_TRUE(result == std::vector<std::string>());

    // one element on vector
    testValues = "A";

    result = yaget::conv::Split(testValues, "");
    EXPECT_TRUE(result == std::vector<std::string>{"A"});

    result = yaget::conv::Split(testValues, ",");
    EXPECT_TRUE(result == std::vector<std::string>{"A"});

    // two element on vector
    testValues = "A,B";

   result = yaget::conv::Split(testValues, "");
   EXPECT_TRUE(result == std::vector<std::string>{"A,B"});

   result = yaget::conv::Split(testValues, ",");
   auto cv = std::vector<std::string>{"A", "B"};
   EXPECT_TRUE(result == cv);

   result = yaget::conv::Split(testValues, ", ");
   EXPECT_TRUE(result == std::vector<std::string>{"A,B"});
}

TEST_F(Strings, NumberConversion)
{
    int64_t result = yaget::conv::AtoN<int64_t>("");
    EXPECT_EQ(0, result);
    result = yaget::conv::AtoN<int64_t>(nullptr);
    EXPECT_EQ(0, result);
    result = yaget::conv::AtoN<int64_t>("f");
    EXPECT_EQ(0, result);
    result = yaget::conv::AtoN<int64_t>("123");
    EXPECT_EQ(123, result);
    result = yaget::conv::AtoN<int64_t>("-123");
    EXPECT_EQ(-123, result);
}

TEST_F(Strings, FilePath)
{
    std::string testPath = "/foo/";
    testPath = yaget::io::NormalizePath(testPath);
    EXPECT_EQ("/foo/", testPath);

    testPath = "\\/\\\\foo\\\\";
    testPath = yaget::io::NormalizePath(testPath);
    EXPECT_EQ("/foo/", testPath);

    testPath = "TestSource@Attach/foo.txt";
    testPath = yaget::io::NormalizePath(testPath);
    EXPECT_EQ("TestSource@Attach/foo.txt", testPath);
    
    testPath = "TestSource@Attach\\foo.txt";
    testPath = yaget::io::NormalizePath(testPath);
    EXPECT_EQ("TestSource@Attach/foo.txt", testPath);
}

