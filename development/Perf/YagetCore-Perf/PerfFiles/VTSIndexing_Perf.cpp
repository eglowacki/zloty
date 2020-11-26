//#include "UnitTest++.h"
#include "VTS/ToolVirtualTransportSystem.h"
#include "VTS/ResolvedAssets.h"
#include "Metrics/Gather.h"

#if 0
#include "Testing/FileTestHarness.h"
#include "Metrics/Concurrency.h"

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

    class BinnerAsset : public yaget::io::Asset
    {
    public:
        BinnerAsset(const yaget::io::Tag& tag, yaget::io::Buffer buffer, const yaget::io::VirtualTransportSystem& vts) : Asset(tag, buffer, vts)
        {}
    };

} // namespace


TEST(VTS_Indexing)
{
    using namespace yaget;
    using Section = io::tool::VirtualTransportSystem::Section;

    metrics::Channel channel("Indexing Lifetime", YAGET_METRICS_CHANNEL_FILE_LINE);

    auto dataBuffer = io::CreateBuffer(417);
    //char data[100];
    const int kMaxCounter = 10000;
    const int kMaxFolders = 10;
    const bool preserveFiles = true;

    const fs::path destFolder = testing::ResetFolder("$(Temp)", preserveFiles);
    const std::string fileNumber = fmt::format("vts_file-{{:0{}}}.bin", GetNumDigits(kMaxCounter));

    if (!preserveFiles)
    {
        metrics::TimeScoper<time::kMilisecondUnit> timeScoper("Files Creation", YAGET_LOG_FILE_LINE_FUNCTION);
        for (int f = 1; f <= kMaxFolders; ++f)
        {
            const std::string folderNumber = fmt::format("vts_folder-{{:0{}}}", GetNumDigits(kMaxFolders));
            const fs::path testFolder = destFolder / fmt::format(folderNumber, f);
            io::file::AssureDirectories(testFolder.generic_string() + "/");

            for (int i = 1; i <= kMaxCounter; ++i)
            {
                const fs::path testFile = testFolder / fmt::format(fileNumber, i);

                io::file::SaveFile(testFile.generic_string(), dataBuffer);
            }
        }
    }

    dev::Configuration::Init::VTSConfigList vtsConfig =
    {
        {
            "BinTester3",
            { "$(Temp)/section-11" },
            { "*.bin" },
            "BINNER",
            false,
            true
        },
        //{
        //    "BinTester2",
        //    { "$(Temp)/section-2" },
        //    { "*.bin" },
        //    "BINNER",
        //    false,
        //    true
        //}
    };

    //dev::Configuration::Init::VTS vtsConfig =
    //{
    //    "BinTester1",
    //    { "$(Temp)/section-1" },
    //    { "*.bin" },
    //    "BINNER",
    //    false,
    //    true
    //};

    io::VirtualTransportSystem::AssetResolvers vtsResolvers =
    {
        { "BINNER", &yaget::io::ResolveAsset<BinnerAsset> }
    };

    metrics::TimeScoper<time::kMilisecondUnit> vtsScoper("VTS Indexing", YAGET_LOG_FILE_LINE_FUNCTION);
    io::tool::VirtualTransportSystemDefault vts(vtsConfig, vtsResolvers, "$(Temp)/vts.sqlite");
}
#endif