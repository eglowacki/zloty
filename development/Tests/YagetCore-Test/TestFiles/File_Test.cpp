#include "pch.h"
#include "App/FileUtilities.h"
#include "Debugging/DevConfiguration.h"
#include "Debugging/DevConfigurationParsers.h"
#include "TestHelpers/TestHelpers.h"


//CHECK_EQUAL(expected, actual);
class File : public ::testing::Test
{
protected:
    // void SetUp() override {}
    // void TearDown() override {}

private:
    //yaget::test::Environment mEnvironment;
};

TEST_F(File, Utilities)
{
    using namespace yaget;

    const char* checkFileName = "$(Temp)/check_file.grb";

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    //--- Save file, check for existence, check file attributes
    auto [result, errorMessage] = io::file::SaveFile(checkFileName, io::CreateBuffer(417));
    EXPECT_TRUE(result);

    result = io::file::IsFileExists(checkFileName);
    EXPECT_TRUE(result);

    result = io::file::IsFileAtrribute(checkFileName, io::file::Attributes::ReadOnly);
    EXPECT_FALSE(result);

    result = io::file::IsFileAtrribute(checkFileName, io::file::Attributes::ReadWrite);
    EXPECT_TRUE(result);

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    //--- set read only, check file attributes, try to save file
    std::tie(result, errorMessage) = io::file::SetFileAtrribute(checkFileName, io::file::Attributes::ReadOnly);
    EXPECT_TRUE(result);

    result = io::file::IsFileAtrribute(checkFileName, io::file::Attributes::ReadOnly);
    EXPECT_TRUE(result);

    result = io::file::IsFileAtrribute(checkFileName, io::file::Attributes::ReadWrite);
    EXPECT_FALSE(result);

    std::tie(result, errorMessage) = io::file::SaveFile(checkFileName, io::CreateBuffer(417));
    EXPECT_FALSE(result);

    //------------------------------------------------------------------------------------------------------------------------------------------------------
    //--- set attributes, check it, save it again, remove file, check for not existing
    std::tie(result, errorMessage) = io::file::SetFileAtrribute(checkFileName, io::file::Attributes::ReadWrite);
    EXPECT_TRUE(result);

    result = io::file::IsFileAtrribute(checkFileName, io::file::Attributes::ReadOnly);
    EXPECT_FALSE(result);

    result = io::file::IsFileAtrribute(checkFileName, io::file::Attributes::ReadWrite);
    EXPECT_TRUE(result);

    std::tie(result, errorMessage) = io::file::SaveFile(checkFileName, io::CreateBuffer(417));
    EXPECT_TRUE(result);

    std::tie(result, errorMessage) = io::file::RemoveFile(checkFileName);
    EXPECT_TRUE(result);

    result = io::file::IsFileExists(checkFileName);
    EXPECT_FALSE(result);
}
