#include "pch.h"

#include "Metrics/Gather.h"
#include "TestHelpers/TestHelpers.h"
#include "VTS/VirtualTransportSystem.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"

YAGET_BRAND_NAME("Beyond Limits")

namespace 
{
    //-------------------------------------------------------------------------------------------------------------------------------
    class TestAsset : public yaget::io::Asset
    {
    public:
        TestAsset(const yaget::io::Tag& tag, yaget::io::Buffer buffer, const yaget::io::VirtualTransportSystem& vts)
            : Asset(tag, buffer, vts) 
            , mMessage(reinterpret_cast<const char*>(buffer.first.get()), buffer.second)
        {
        }

        std::string mMessage;
    };

    //-------------------------------------------------------------------------------------------------------------------------------
    yaget::io::VirtualTransportSystem::AssetResolvers Resolvers = {
        { "TEST", yaget::io::ResolveAsset<TestAsset> },
        { "JSON", yaget::io::ResolveAsset<TestAsset> }//yaget::io::ResolveJsonAsset }
    };


    const auto configBlock = R"###(
        {
            "Configuration" : {
                "Init" : {
                    "Aliases": {
                       "$(AssetsFolder)": {
                            "Path": "$(UserDataFolder)/Assets",
                            "ReadOnly" : true
                        },
                       "$(DatabaseFolder) ": {
                            "Path": "$(UserDataFolder)/Database",
                            "ReadOnly" : true
                        }
                    },
                    "VTS" : [{
                        "SourceDocs": {
                            "Converters": "TEST",
                            "Filters" : [ "*.txt" ],
                            "Path" : [ "$(AssetsFolder)/Sources" ],
                            "ReadOnly" : true,
                            "Recursive" : true
                        }
                    },
                    {
                        "TargetDocs": {
                            "Converters": "TEST",
                            "Filters" : [ "*.txt" ],
                            "Path" : [ "$(AssetsFolder)/Targets" ],
                            "ReadOnly" : false,
                            "Recursive" : true
                        }
                    },
                    {
                        "WriteTestSettings": {
                            "Converters": "TEST",
                            "Filters" : [ "*.txt" ],
                            "Path" : [ "$(AssetsFolder)" ],
                            "ReadOnly" : false,
                            "Recursive" : true
                        }
                    },
                    {
                        "TestSettings": {
                            "Path": [ "$(AssetsFolder)/Settings", "$(AssetsFolder)/User/Settings" ],
                            "ReadOnly": true,
                            "Filters": [ "*.txt" ],
                            "Converters": "TEST"
                        }
                    }]
                }
            }
        }
    )###";

} // namespace


class VTS : public ::testing::Test
{
protected:
    //void SetUp() override
    //void TearDown() override

private:
    yaget::test::Environment mEnvironment{ configBlock, std::strlen(configBlock) };
};

TEST_F(VTS, Section)
{
    using namespace yaget;
    using Section = io::VirtualTransportSystem::Section;

    Section section("TestSource@Attach/foo.txt");
    EXPECT_EQ(section.Name, "TestSource");
    EXPECT_EQ(section.Filter, "Attach/foo.txt");
    EXPECT_TRUE(section.Match == Section::FilterMatch::Like);
    EXPECT_EQ("TestSource@Attach/foo.txt", section.ToString());

    section = Section("=TestSource@Attach/foo.txt");
    EXPECT_EQ(section.Name, "TestSource");
    EXPECT_EQ(section.Filter, "Attach/foo.txt");
    EXPECT_TRUE(section.Match == Section::FilterMatch::Exact);
    EXPECT_EQ("=TestSource@Attach/foo.txt", section.ToString());

    section = Section(">TestSource@Attach/foo.txt");
    EXPECT_EQ(section.Name, "TestSource");
    EXPECT_EQ(section.Filter, "Attach/foo.txt");
    EXPECT_TRUE(section.Match == Section::FilterMatch::Override);
    EXPECT_EQ(">TestSource@Attach/foo.txt", section.ToString());

    section = Section("TestSource@");
    EXPECT_EQ(section.Name, "TestSource");
    EXPECT_EQ(section.Filter, "");
    EXPECT_TRUE(section.Match == Section::FilterMatch::Like);
    EXPECT_EQ("TestSource", section.ToString());

    section = Section(">TestSource@");
    EXPECT_EQ(section.Name, "TestSource");
    EXPECT_EQ(section.Filter, "");
    EXPECT_TRUE(section.Match == Section::FilterMatch::Like);
    EXPECT_EQ("TestSource", section.ToString());

    section = Section("TestSource");
    EXPECT_EQ(section.Name, "TestSource");
    EXPECT_EQ(section.Filter, "");
    EXPECT_TRUE(section.Match == Section::FilterMatch::Like);
    EXPECT_EQ("TestSource", section.ToString());

    section = Section(">TestSource");
    EXPECT_EQ(section.Name, "TestSource");
    EXPECT_EQ(section.Filter, "");
    EXPECT_TRUE(section.Match == Section::FilterMatch::Like);
    EXPECT_EQ("TestSource", section.ToString());

    section = Section("@TestSource");
    EXPECT_EQ(section.Name, "");
    EXPECT_EQ(section.Filter, "");
    EXPECT_TRUE(section.Match == Section::FilterMatch::Like);
    EXPECT_EQ("", section.ToString());

    section = Section(">@TestSource");
    EXPECT_EQ(section.Name, "");
    EXPECT_EQ(section.Filter, "");
    EXPECT_TRUE(section.Match == Section::FilterMatch::Like);
    EXPECT_EQ("", section.ToString());

    section = Section("");
    EXPECT_EQ(section.Name, "");
    EXPECT_EQ(section.Filter, "");
    EXPECT_TRUE(section.Match == Section::FilterMatch::Like);
    EXPECT_EQ("", section.ToString());

    section = Section(">");
    EXPECT_EQ(section.Name, "");
    EXPECT_EQ(section.Filter, "");
    EXPECT_TRUE(section.Match == Section::FilterMatch::Like);
    EXPECT_EQ("", section.ToString());

    Section initialSection = Section(">TestSource@Attach/foo.txt");
    nlohmann::json rootData(initialSection);

    Section newSection = rootData.get<Section>();
    EXPECT_EQ(initialSection.ToString(), newSection.ToString());

    initialSection = Section();
    rootData = nlohmann::json(initialSection);

    newSection = rootData.get<Section>();
    EXPECT_EQ(initialSection.ToString(), newSection.ToString());
}


TEST_F(VTS, TransportSystem)
{
    using namespace yaget;
    using Options = io::tool::VirtualTransportSystem::Options;
    using Section = io::VirtualTransportSystem::Section;

    const char* vtsFile = "$(DatabaseFolder)/vts.sqlite";
    const Section blobFile("SourceDocs@Attach/foo.txt");
    const Section sourceSection("SourceDocs@Attach");
    const Section targetSection("TargetDocs@Clones");
    const std::string flatCopy = targetSection.Filter + "/foo.txt";
    const std::string exactCopy = targetSection.Filter + "/" + blobFile.Filter;
    const std::string message = "Hello World";
    io::Tag newTag;
    io::Tag flatTag, exactTag;

    io::tool::VirtualTransportSystemReset vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

    //--------------------------------------------------------------------------------------------------
    // create new buffer, attach it to vts, load it back up and then save it
    {
        metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", "VTS Test 1", YAGET_LOG_FILE_LINE_FUNCTION);

        // delete all test data,
        EXPECT_TRUE(vts.DeleteBlob({ sourceSection, targetSection }));
        EXPECT_EQ(vts.GetNumTags({ sourceSection, targetSection }), 0);

        newTag = vts.GenerateTag(blobFile);
        EXPECT_TRUE(newTag.mGuid.IsValid());

        std::shared_ptr<TestAsset> testAsset = std::make_shared<TestAsset>(newTag, io::CreateBuffer(message), vts);
        EXPECT_EQ(testAsset->mMessage, message);

        EXPECT_TRUE(vts.AttachBlob(testAsset));

        EXPECT_EQ(vts.GetNumTags(blobFile), 1);
        EXPECT_EQ(vts.GetNumTags(Section("SourceDocs@Attach/FOO.txt")), 1);   // check for case insensitive in Filter part
        EXPECT_EQ(vts.GetNumTags(Section("SourceDocs@Attach/fOO.tXT")), 1);

        io::BLobLoader<TestAsset> bLobLoader(vts, blobFile);
        auto checkedAsset = bLobLoader.Assets();
        EXPECT_EQ(checkedAsset.size(), 1);

        auto asset = *checkedAsset.begin();
        EXPECT_EQ(asset->mMessage, message);
    }
    
    //--------------------------------------------------------------------------------------------------
    // Find attached blob with correct guid, copy to new section, flat and preserve hierarchy and save
    {
        metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", "VTS Test 2", YAGET_LOG_FILE_LINE_FUNCTION);

        EXPECT_EQ(vts.GetNumTags(blobFile), 1);

        io::BLobLoader<TestAsset> bLobLoader(vts, blobFile);
        auto checkedAsset = bLobLoader.Assets();
        EXPECT_EQ(checkedAsset.size(), 1);
        auto asset = *checkedAsset.begin();
        EXPECT_TRUE(asset->mTag.mGuid == newTag.mGuid && asset->mMessage == message);

        io::Buffer sourceBuffer = asset->mBuffer;

        flatTag = vts.CopyTag(newTag, targetSection, Options::Flat);
        EXPECT_TRUE(flatTag.mGuid.IsValid());
        io::Buffer flatData = io::CloneBuffer(sourceBuffer);
        std::shared_ptr<TestAsset> flatAsset = std::make_shared<TestAsset>(flatTag, flatData, vts);
        EXPECT_EQ(flatAsset->mMessage, message);
        EXPECT_TRUE(vts.AttachBlob(flatAsset));

        exactTag = vts.CopyTag(newTag, targetSection, Options::Hierarchy);
        EXPECT_TRUE(exactTag.mGuid.IsValid());
        io::Buffer exactData = io::CloneBuffer(sourceBuffer);
        std::shared_ptr<TestAsset> exactAsset = std::make_shared<TestAsset>(exactTag, exactData, vts);
        EXPECT_EQ(exactAsset->mMessage, message);
        EXPECT_TRUE(vts.AttachBlob(exactAsset));
    }

    //--------------------------------------------------------------------------------------------------
    // delete attached blob
    {
        metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", "VTS Test 3", YAGET_LOG_FILE_LINE_FUNCTION);

        io::BLobLoader<TestAsset> flatLoader(vts, Section(targetSection.Name + "@" + flatCopy));
        EXPECT_TRUE(!flatLoader.Assets().empty() && (*flatLoader.Assets().begin())->mTag.mGuid == flatTag.mGuid);

        io::BLobLoader<TestAsset> exactLoader(vts, Section(targetSection.Name + "@" + exactCopy));
        EXPECT_TRUE(exactLoader.Assets().empty() == false && (*exactLoader.Assets().begin())->mTag.mGuid == exactTag.mGuid);

        EXPECT_TRUE(vts.DeleteBlob({ sourceSection, targetSection }));
        EXPECT_EQ(vts.GetNumTags({ sourceSection, targetSection }), 0);
    }

    //--------------------------------------------------------------------------------------------------
    // recover guid from deleted blob
    {
        metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", "VTS Test 4", YAGET_LOG_FILE_LINE_FUNCTION);

        io::Tag recoveredTag = vts.GenerateTag(blobFile);
        EXPECT_EQ(newTag.mGuid, recoveredTag.mGuid);
        io::Tag unrecoveredTag = vts.GenerateTag(blobFile);
        EXPECT_TRUE(unrecoveredTag.mGuid != newTag.mGuid);
    }

    //--------------------------------------------------------------------------------------------------
    const Section file11(sourceSection.ToString() + "/Folder1/File1.txt");
    const Section file12(sourceSection.ToString() + "/Folder1/File2.txt");
    const Section file13(sourceSection.ToString() + "/Folder1/File3.txt");
    const Section file21(sourceSection.ToString() + "/Folder2/File1.txt");
    const Section file22(sourceSection.ToString() + "/Folder2/File2.txt");

    const Section file1A(sourceSection.ToString() + "/Folder1/A/Budapest.txt");
    const Section file2A(sourceSection.ToString() + "/Folder2/A/Moscow.txt");
    // create ab folder structure and files based on above Sections
    {
        metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", "VTS Test 5", YAGET_LOG_FILE_LINE_FUNCTION);

        io::Tag copyTag = vts.GenerateTag(file11);
        io::Buffer data = io::CreateBuffer("Folder1/File1.txt");
        EXPECT_TRUE(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file12);
        data = io::CreateBuffer("Folder1/File2.txt");
        EXPECT_TRUE(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file13);
        data = io::CreateBuffer("Folder1/File3.txt");
        EXPECT_TRUE(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file21);
        data = io::CreateBuffer("Folder2/File1.txt");
        EXPECT_TRUE(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file22);
        data = io::CreateBuffer("Folder2/File2.txt");
        EXPECT_TRUE(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file1A);
        data = io::CreateBuffer("Folder1/A/Budapest.txt");
        EXPECT_TRUE(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file2A);
        data = io::CreateBuffer("Folder2/A/Moscow.txt");
        EXPECT_TRUE(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));
    }

    //--------------------------------------------------------------------------------------------------
    // copy above tree as is, preserving folder structure
    {
        metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", "VTS Test 6", YAGET_LOG_FILE_LINE_FUNCTION);

        io::BLobLoader<TestAsset> bLobLoader(vts, sourceSection);
        for (const auto& it : bLobLoader.Assets())
        {
            io::Tag copiedTag = vts.CopyTag(it->mTag, targetSection, Options::Hierarchy);
            io::Buffer data = io::CloneBuffer(it->mBuffer);
            EXPECT_TRUE(vts.AttachBlob(std::make_shared<TestAsset>(copiedTag, data, vts)));
        }

        EXPECT_EQ(vts.GetNumTags(targetSection), bLobLoader.Assets().size());
        EXPECT_TRUE(vts.DeleteBlob(targetSection));
        EXPECT_EQ(vts.GetNumTags(targetSection), 0);
    }

    //--------------------------------------------------------------------------------------------------
    // copy above tree as flat, all in one folder under sourceSection, it should fail due to same file names in different folders
    {
        metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", "VTS Test 7", YAGET_LOG_FILE_LINE_FUNCTION);

        std::vector<std::shared_ptr<io::Asset>> assets;
        io::BLobLoader<TestAsset> bLobLoader(vts, sourceSection);
        for (const auto& it : bLobLoader.Assets())
        {
            io::Tag copiedTag = vts.CopyTag(it->mTag, targetSection, Options::Flat);
            io::Buffer data = io::CloneBuffer(it->mBuffer);
            assets.push_back(std::make_shared<TestAsset>(copiedTag, data, vts));
        }

        EXPECT_FALSE(vts.AttachBlob(assets));
        EXPECT_EQ(vts.GetNumTags(targetSection), 0);
    }

    //--------------------------------------------------------------------------------------------------
    // copy above tree as flat, but remove duplicate first, this should succeed
    {
        metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", "VTS Test 8", YAGET_LOG_FILE_LINE_FUNCTION);

        EXPECT_TRUE(vts.DeleteBlob({ file11, file12 }));

        std::vector<std::shared_ptr<io::Asset>> assets;
        io::BLobLoader<TestAsset> bLobLoader(vts, sourceSection);
        for (const auto& it : bLobLoader.Assets())
        {
            io::Tag copiedTag = vts.CopyTag(it->mTag, targetSection, Options::Flat);
            io::Buffer data = io::CloneBuffer(it->mBuffer);
            assets.push_back(std::make_shared<TestAsset>(copiedTag, data, vts));
        }

        EXPECT_TRUE(vts.AttachBlob(assets));
        EXPECT_EQ(vts.GetNumTags(targetSection), bLobLoader.Assets().size());
    }

    //--------------------------------------------------------------------------------------------------
    // cleanup everything
    {
        metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", "VTS Test 9", YAGET_LOG_FILE_LINE_FUNCTION);

        EXPECT_TRUE(vts.DeleteBlob({ sourceSection, targetSection }));
        EXPECT_EQ(vts.GetNumTags({ sourceSection, targetSection }), 0);
    }

    //--------------------------------------------------------------------------------------------------
    const Section file1(sourceSection.ToString() + "/File.txt");
    const Section file2(sourceSection.ToString() + "/File2.txt");
    const Section file3(sourceSection.ToString() + "/File_foo.txt");
    const Section file4(sourceSection.ToString() + "/Varoom.txt");
    // create files for search/find test
    {
        metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", "VTS Test 10", YAGET_LOG_FILE_LINE_FUNCTION);
        //io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        io::Tag copyTag = vts.GenerateTag(file1);
        io::Buffer data = io::CreateBuffer("xxx");
        EXPECT_TRUE(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file2);
        data = io::CreateBuffer("xxx");
        EXPECT_TRUE(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file3);
        data = io::CreateBuffer("xxx");
        EXPECT_TRUE(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file4);
        data = io::CreateBuffer("xxx");
        EXPECT_TRUE(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));
    }

    //--------------------------------------------------------------------------------------------------
    // test finding partial match and exact match
    {
        metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", "VTS Test 11", YAGET_LOG_FILE_LINE_FUNCTION);
        //io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        Section finder(sourceSection.ToString() + "/File");
        EXPECT_EQ(vts.GetNumTags(finder), 3);

        finder = Section(sourceSection.ToString() + "/Varoom");
        EXPECT_EQ(vts.GetNumTags(finder), 1);

        finder = Section(sourceSection.ToString() + "/File3");
        EXPECT_EQ(vts.GetNumTags(finder), 0);
    }

    //--------------------------------------------------------------------------------------------------
    const Section settings("WriteTestSettings");
    // create files to test override
    {
        metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", "VTS Test 12", YAGET_LOG_FILE_LINE_FUNCTION);
        //io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        EXPECT_TRUE(vts.DeleteBlob(settings));
        EXPECT_EQ(vts.GetNumTags(settings), 0);

        const Section settingsFile1("WriteTestSettings@Settings/Bindings.txt");
        io::Tag tag1 = vts.GenerateTag(settingsFile1);
        io::Buffer data1 = io::CreateBuffer("Exit App");
        EXPECT_TRUE(vts.AttachBlob(std::make_shared<TestAsset>(tag1, data1, vts)));

        const Section settingsFile2("WriteTestSettings@User/Settings/Bindings.txt");
        io::Tag tag2 = vts.GenerateTag(settingsFile2);
        io::Buffer data2 = io::CreateBuffer("Sound Options");
        EXPECT_TRUE(vts.AttachBlob(std::make_shared<TestAsset>(tag2, data2, vts)));
    }

    //--------------------------------------------------------------------------------------------------
    // get override settings file
    {
        metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", "VTS Test 13", YAGET_LOG_FILE_LINE_FUNCTION);
        //io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        const Section settingsFile(">TestSettings@Bindings");

        io::BLobLoader<TestAsset> settingLoader(vts, settingsFile);
        EXPECT_EQ(settingLoader.Assets().size(), 1);
        auto settingAsset = *settingLoader.Assets().begin();
        EXPECT_EQ(settingAsset->mMessage, "Sound Options");

        const Section settingsFiles("TestSettings@Bindings");

        io::BLobLoader<TestAsset> settingsLoader(vts, settingsFiles);
        EXPECT_EQ(settingsLoader.Assets().size(), 2);
        auto asset0 = settingsLoader.Assets()[0];
        EXPECT_EQ(asset0->mMessage, "Sound Options");
        auto asset1 = settingsLoader.Assets()[1];
        EXPECT_EQ(asset1->mMessage, "Exit App");

        {
            std::vector<io::Tag> tags = vts.GetTags(settingsFile);
            EXPECT_EQ(tags.size(), 1);
            io::Tag cachedTag = *tags.begin();

            io::Buffer buffer = io::CreateBuffer("Cached Sound Options");
            auto cachedAsset = std::make_shared<TestAsset>(cachedTag, buffer, vts);
            vts.AddOverride(cachedAsset);
        }

        io::BLobLoader<TestAsset> cachedLoader(vts, settingsFile);
        EXPECT_EQ(cachedLoader.Assets().size(), 1);
        auto cachedAsset = *cachedLoader.Assets().begin();
        EXPECT_EQ(cachedAsset->mMessage, "Cached Sound Options");
    }

    //--------------------------------------------------------------------------------------------------
    // cleanup settings folder
    {
        metrics::TimeScoper<time::kMilisecondUnit> intTimer("TEST", "VTS Test 14", YAGET_LOG_FILE_LINE_FUNCTION);
        //io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        EXPECT_TRUE(vts.DeleteBlob({ settings, sourceSection }));
        EXPECT_EQ(vts.GetNumTags({ settings, sourceSection }), 0);
    }

}
