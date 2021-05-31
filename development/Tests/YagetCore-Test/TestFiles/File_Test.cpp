#include "pch.h"
#include "App/FileUtilities.h"
#include "Debugging/DevConfiguration.h"
#include "Debugging/DevConfigurationParsers.h"
#include "TestHelpers/TestHelpers.h"

#include <filesystem>
namespace fs = std::filesystem;

//CHECK_EQUAL(expected, actual);
class File : public ::testing::Test
{
private:
    yaget::test::Environment mEnvironment;
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


TEST_F(File, LogCycler)
{
    using namespace yaget;
    namespace FileComp = yaget::io::file::FileComp;

    const char* testFileName = "$(Temp)/LogCycler.garbage";
    const char* filter = "*LogCycler*.garbage";
    const io::file::FileComponents comps = io::file::SplitComponents(testFileName);

    const char* filePath = std::get<FileComp::Path>(comps).c_str();
    const char* fileName = std::get<FileComp::File>(comps).c_str();
    const char* fileExtension = std::get<FileComp::Extension>(comps).c_str();

    EXPECT_STREQ("$(Temp)", filePath);
    EXPECT_STREQ("LogCycler", fileName);
    EXPECT_STREQ(".garbage", fileExtension);

    const auto filePathName = fs::path(yaget::util::ExpendEnv(testFileName, nullptr)).generic_string();
    Strings oldFiles = io::file::GetFileNames(yaget::util::ExpendEnv(filePath, nullptr), false, filter);
    io::file::RemoveFiles(oldFiles);

    EXPECT_TRUE(util::FileCycler(filePath, fileName, fileExtension));

    EXPECT_EQ(0, io::file::GetFileNames(yaget::util::ExpendEnv(filePath, nullptr), false, filter).size());

    const auto dataBuffer = io::CreateBuffer("417");
    io::file::SaveFile(filePathName, dataBuffer);

    oldFiles = io::file::GetFileNames(yaget::util::ExpendEnv(filePath, nullptr), false, filter);
    EXPECT_EQ(1, oldFiles.size());

    EXPECT_TRUE(util::FileCycler(filePath, fileName, fileExtension));
    io::file::SaveFile(filePathName, dataBuffer);
    oldFiles = io::file::GetFileNames(yaget::util::ExpendEnv(filePath, nullptr), false, filter);
    EXPECT_EQ(2, oldFiles.size());

    EXPECT_TRUE(util::FileCycler(filePath, fileName, fileExtension));
    io::file::SaveFile(filePathName, dataBuffer);
    oldFiles = io::file::GetFileNames(yaget::util::ExpendEnv(filePath, nullptr), false, filter);
    EXPECT_EQ(3, oldFiles.size());

    EXPECT_TRUE(util::FileCycler(filePath, fileName, fileExtension));
    io::file::SaveFile(filePathName, dataBuffer);
    oldFiles = io::file::GetFileNames(yaget::util::ExpendEnv(filePath, nullptr), false, filter);
    EXPECT_EQ(4, oldFiles.size());

    EXPECT_TRUE(util::FileCycler(filePath, fileName, fileExtension, 1));
    oldFiles = io::file::GetFileNames(yaget::util::ExpendEnv(filePath, nullptr), false, filter);
    EXPECT_EQ(1, oldFiles.size());

    io::file::RemoveFiles(oldFiles);
}
