#include "pch.h"
#include "VTS/BlobLoader.h"
#include "Fmt/format.h"
#include "StringHelpers.h"
#include "App/FileUtilities.h"
#include "TestHelpers/TestHelpers.h"
#include "Metrics/Concurrency.h"

#include "Platform/Support.h"

#include <filesystem>
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

    void CleanTestFiles()
    {
        using namespace yaget;

        const fs::path destFolder = util::ExpendEnv("$(Temp)", nullptr);

        const std::string fileFilter = { "*blob_file-*.bin" };
        const Strings oldFiles = io::file::GetFileNames(destFolder.generic_string(), false, fileFilter);
        io::file::RemoveFiles(oldFiles);
    }

    yaget::Strings CleanupAndSetup(int maxNumFiles)
    {
        using namespace yaget;

        constexpr size_t fileSize = 1024 * 1024 * 10;
        metrics::Channel channel(fmt::format("CleanupAndSetup '{}' files", maxNumFiles), YAGET_METRICS_CHANNEL_FILE_LINE);

        CleanTestFiles();

        const fs::path destFolder = util::ExpendEnv("$(Temp)", nullptr);
        const std::string fileNumber = fmt::format("blob_file-{{:0{}}}.bin", GetNumDigits(maxNumFiles));

        const auto dataBuffer = io::CreateBuffer(fileSize);

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

    metrics::Channel channel("Main.BlobLoader", YAGET_METRICS_CHANNEL_FILE_LINE);

    const int kMaxNumFiles = 90;
    const Strings filesToTest = CleanupAndSetup(kMaxNumFiles);

    int counter = 0;
    {
        metrics::Channel vChannel("Input Validation", YAGET_METRICS_CHANNEL_FILE_LINE);

        io::BlobLoader blobLoader(true, {});

        // first, validate some input assumptions and make sure that loader throws on error
        auto testConverters = std::vector<io::BlobLoader::Convertor>{};

        EXPECT_NO_THROW(blobLoader.AddTask({}, testConverters));

        EXPECT_ANY_THROW(blobLoader.AddTask(filesToTest, testConverters));

        testConverters.push_back({});
        testConverters.push_back({});
        EXPECT_ANY_THROW(blobLoader.AddTask(filesToTest, testConverters));
    }

    {
        metrics::Channel vChannel("Process All Requests", YAGET_METRICS_CHANNEL_FILE_LINE);

        io::BlobLoader blobLoader(true, {});

        EXPECT_NO_THROW(blobLoader.AddTask(filesToTest, [&counter](const auto& fileData)
        {
            ++counter;
            platform::BusySleep(20, time::kMilisecondUnit);
        }));
    }

    EXPECT_EQ(counter, kMaxNumFiles);

    counter = 0;
    {
        metrics::Channel aChannel("Cancel All Requests", YAGET_METRICS_CHANNEL_FILE_LINE);

        io::BlobLoader blobLoader(false, {});

        blobLoader.AddTask(filesToTest, [&counter](const auto& fileData)
        {
            ++counter;
            platform::BusySleep(1, time::kMilisecondUnit);
        });
    }
    EXPECT_LT(counter, kMaxNumFiles);

    CleanTestFiles();
}
