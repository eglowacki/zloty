#include "pch.h"
#include "VTS/BlobLoader.h"
#include "Fmt/format.h"
#include "StringHelpers.h"
#include "App/FileUtilities.h"
#include "TestHelpers/TestHelpers.h"

#include <filesystem>

#include "Platform/Support.h"
namespace fs = std::filesystem;

namespace
{
    int GetNumDigits(int value)
    {
        int length = 1;
        while (value /= 10)
        {
            length++;
        }

        return length;
    }
    yaget::Strings CleanupAndSetup(int maxNumFiles)
    {
        using namespace yaget;

        const fs::path destFolder = util::ExpendEnv("$(Temp)", nullptr);
        const std::string fileNumber = fmt::format("blob_file-{{:0{}}}.bin", GetNumDigits(maxNumFiles));

        const std::string fileFilter = { "*blob_file-*.bin" };
        const Strings oldFiles = io::file::GetFileNames(destFolder.generic_string(), false, fileFilter);
        io::file::RemoveFiles(oldFiles);

        const auto dataBuffer = io::CreateBuffer("417");

        Strings filesToTest;
        for (int i = 0; i < maxNumFiles; ++i)
        {
            const fs::path blobFile = destFolder / fmt::format(fileNumber, i);
            filesToTest.emplace_back(blobFile.generic_string());
            io::file::SaveFile(filesToTest.back(), dataBuffer);
        }

        return filesToTest;
    }
}

//CHECK_EQUAL(expected, actual);
class BlobLoader : public ::testing::Test
{
private:
    yaget::test::Environment mEnvironment;
};

TEST_F(BlobLoader, LoadConvert)
{
    using namespace yaget;

    const int kMaxNumFiles = 100;

    const Strings filesToTest = CleanupAndSetup(kMaxNumFiles);

    int counter = 0;
    {
        io::BlobLoader blobLoader(true, nullptr);

        // first, validate some input assumptions and make sure that loader
        // throws on error
        auto testConverters = std::vector<io::BlobLoader::Convertor>{};

        EXPECT_NO_THROW(blobLoader.AddTask({}, testConverters));

        EXPECT_ANY_THROW(blobLoader.AddTask(filesToTest, testConverters));

        testConverters.push_back({});
        testConverters.push_back({});
        EXPECT_ANY_THROW(blobLoader.AddTask(filesToTest, testConverters));

        YLOG_NOTICE("TEST", "There are '%d' files to process.", filesToTest.size());
        EXPECT_NO_THROW(blobLoader.AddTask(filesToTest, [&counter](const auto& fileData)
        {
                ++counter;
        }));
    }
    EXPECT_EQ(counter, kMaxNumFiles);

    counter = 0;
    {
        io::BlobLoader blobLoader(false, nullptr);

        YLOG_NOTICE("TEST", "There are '%d' files to process.", filesToTest.size());
        blobLoader.AddTask(filesToTest, [&counter](const auto& fileData)
        {
            ++counter;
        });
    }
    EXPECT_LE(counter, kMaxNumFiles);
}
