#include "pch.h"

#include "TestHelpers/TestHelpers.h"
#include "VTS/VirtualTransportSystem.h"
#include "VTS/ResolvedAssets.h"
#include "VTS/ToolVirtualTransportSystem.h"

namespace 
{
    //-------------------------------------------------------------------------------------------------------------------------------
    class TestAsset : public yaget::io::Asset
    {
    public:
        TestAsset(const yaget::io::Tag& tag, yaget::io::Buffer buffer, yaget::io::VirtualTransportSystem& vts)
            : Asset(tag, buffer, vts) 
            , mMessage(reinterpret_cast<const char*>(buffer.first.get()), buffer.second)
        {
        }

        std::string mMessage;
    };

    //-------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<yaget::io::Asset> ResolveTestAsset(const yaget::io::Buffer& dataBuffer, const yaget::io::Tag& requestedTag, yaget::io::VirtualTransportSystem& vts)
    {
        std::shared_ptr<TestAsset> testAsset = std::make_shared<TestAsset>(requestedTag, dataBuffer, vts);
        return testAsset->IsValid() ? testAsset : nullptr;
    }

    //-------------------------------------------------------------------------------------------------------------------------------
    yaget::io::VirtualTransportSystem::AssetResolvers Resolvers = {
        { "TEST", ResolveTestAsset },
        { "JSON", ResolveTestAsset }//yaget::io::ResolveJsonAsset }
    };

} // namespace


class VTS : public ::testing::Test
{
protected:
    // void SetUp() override {}
    // void TearDown() override {}

private:
    yaget::test::Environment mEnvironment;
};

TEST(VTS, Section)
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


TEST(VTS, TransportSystem)
{
    using namespace yaget;
    using Options = io::tool::VirtualTransportSystem::Options;
    using Section = io::VirtualTransportSystem::Section;

    const char* vtsFile = "$(AppDataFolder)/Database/vts.sqlite";
    const Section blobFile("TestSource@Attach/foo.txt");
    const Section sourceSection("TestSource@Attach");
    const Section targetSection("TestTarget@Clones");
    const std::string flatCopy = targetSection.Filter + "/foo.txt";
    const std::string exactCopy = targetSection.Filter + "/" + blobFile.Filter;
    const std::string message = "Hello World";
    io::Tag newTag;
    io::Tag flatTag, exactTag;
    
    //--------------------------------------------------------------------------------------------------
    // create new buffer, attach it to vts, load it back up and then save it
    {
        io::tool::VirtualTransportSystemReset vts({}, Resolvers, vtsFile);

        //// delete all test data,
        //CHECK(vts.DeleteBlob({ sourceSection, targetSection }));
        //CHECK_EQUAL(vts.GetNumTags({ sourceSection, targetSection }), 0);

        //newTag = vts.GenerateTag(blobFile);
        //CHECK(newTag.mGuid.IsValid());

        //std::shared_ptr<TestAsset> testAsset = std::make_shared<TestAsset>(newTag, io::CreateBuffer(message), vts);
        //CHECK_EQUAL(message, testAsset->mMessage);

        //CHECK(vts.AttachBlob(testAsset));

        //CHECK_EQUAL(1, vts.GetNumTags(blobFile));
        //CHECK_EQUAL(1, vts.GetNumTags(Section("TestSource@Attach/FOO.txt")));   // check for case insensitive in Filter part
        //CHECK_EQUAL(1, vts.GetNumTags(Section("TestSource@Attach/fOO.tXT")));

        //io::BLobLoader<TestAsset> bLobLoader(vts, blobFile);
        //auto checkedAsset = bLobLoader.Assets();
        //CHECK_EQUAL(1, checkedAsset.size());

        //auto asset = *checkedAsset.begin();
        //CHECK_EQUAL(message, asset->mMessage);
    }
    
#if 0
    //--------------------------------------------------------------------------------------------------
    // Find attached blob with correct guid, copy to new section, flat and preserve hierarchy and save
    {
        io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        CHECK_EQUAL(1, vts.GetNumTags(blobFile));

        io::BLobLoader<TestAsset> bLobLoader(vts, blobFile);
        auto checkedAsset = bLobLoader.Assets();
        CHECK_EQUAL(1, checkedAsset.size());
        auto asset = *checkedAsset.begin();
        CHECK(asset->mTag.mGuid == newTag.mGuid && asset->mMessage == message);

        //auto it = checkedAsset.find(newTag.mGuid);
        //CHECK(it != checkedAsset.end() && it->first == newTag.mGuid && it->second->mMessage == message);

        io::Buffer sourceBuffer = asset->mBuffer;

        flatTag = vts.CopyTag(newTag, targetSection, Options::Flat);
        CHECK(flatTag.mGuid.IsValid());
        io::Buffer flatData = io::CloneBuffer(sourceBuffer);
        std::shared_ptr<TestAsset> flatAsset = std::make_shared<TestAsset>(flatTag, flatData, vts);
        CHECK_EQUAL(message, flatAsset->mMessage);
        CHECK(vts.AttachBlob(flatAsset));

        exactTag = vts.CopyTag(newTag, targetSection, Options::Hierarchy);
        CHECK(exactTag.mGuid.IsValid());
        io::Buffer exactData = io::CloneBuffer(sourceBuffer);
        std::shared_ptr<TestAsset> exactAsset = std::make_shared<TestAsset>(exactTag, exactData, vts);
        CHECK_EQUAL(message, exactAsset->mMessage);
        CHECK(vts.AttachBlob(exactAsset));
    }

    //--------------------------------------------------------------------------------------------------
    // delete attached blob
    {
        io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        io::BLobLoader<TestAsset> flatLoader(vts, Section(targetSection.Name + "@" + flatCopy));
        CHECK(!flatLoader.Assets().empty() && (*flatLoader.Assets().begin())->mTag.mGuid == flatTag.mGuid);

        io::BLobLoader<TestAsset> exactLoader(vts, Section(targetSection.Name + "@" + exactCopy));
        CHECK(exactLoader.Assets().empty() == false && (*exactLoader.Assets().begin())->mTag.mGuid == exactTag.mGuid);

        CHECK(vts.DeleteBlob({ sourceSection, targetSection }));
        CHECK_EQUAL(0, vts.GetNumTags({ sourceSection, targetSection }));
    }
    
    //--------------------------------------------------------------------------------------------------
    // recover guid from deleted blob
    {
        io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        io::Tag recoveredTag = vts.GenerateTag(blobFile);
        CHECK_EQUAL(newTag.mGuid, recoveredTag.mGuid);
        io::Tag unrecoveredTag = vts.GenerateTag(blobFile);
        CHECK(unrecoveredTag.mGuid != newTag.mGuid);
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
        io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        io::Tag copyTag = vts.GenerateTag(file11);
        io::Buffer data = io::CreateBuffer("Folder1/File1.txt");
        CHECK(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file12);
        data = io::CreateBuffer("Folder1/File2.txt");
        CHECK(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file13);
        data = io::CreateBuffer("Folder1/File3.txt");
        CHECK(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file21);
        data = io::CreateBuffer("Folder2/File1.txt");
        CHECK(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file22);
        data = io::CreateBuffer("Folder2/File2.txt");
        CHECK(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file1A);
        data = io::CreateBuffer("Folder1/A/Budapest.txt");
        CHECK(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file2A);
        data = io::CreateBuffer("Folder2/A/Moscow.txt");
        CHECK(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));
    }

    //--------------------------------------------------------------------------------------------------
    // copy above tree as is, preserving folder structure
    {
        io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        io::BLobLoader<TestAsset> bLobLoader(vts, sourceSection);
        for (const auto& it : bLobLoader.Assets())
        {
            io::Tag copiedTag = vts.CopyTag(it->mTag, targetSection, Options::Hierarchy);
            io::Buffer data = io::CloneBuffer(it->mBuffer);
            CHECK(vts.AttachBlob(std::make_shared<TestAsset>(copiedTag, data, vts)));
        }

        CHECK_EQUAL(bLobLoader.Assets().size(), vts.GetNumTags(targetSection));
        CHECK(vts.DeleteBlob(targetSection));
        CHECK_EQUAL(0, vts.GetNumTags(targetSection));
    }

    //--------------------------------------------------------------------------------------------------
    // copy above tree as flat, all in one folder under sourceSection, it should fail due to same file names in different folders
    {
        io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        std::vector<std::shared_ptr<io::Asset>> assets;
        io::BLobLoader<TestAsset> bLobLoader(vts, sourceSection);
        for (const auto& it : bLobLoader.Assets())
        {
            io::Tag copiedTag = vts.CopyTag(it->mTag, targetSection, Options::Flat);
            io::Buffer data = io::CloneBuffer(it->mBuffer);
            assets.push_back(std::make_shared<TestAsset>(copiedTag, data, vts));
        }

        CHECK_EQUAL(false, vts.AttachBlob(assets));
        CHECK_EQUAL(0, vts.GetNumTags(targetSection));
    }

    //--------------------------------------------------------------------------------------------------
    // copy above tree as flat, but remove duplicate first, this should succeed
    {
        io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        CHECK(vts.DeleteBlob({ file11, file12 }));

        std::vector<std::shared_ptr<io::Asset>> assets;
        io::BLobLoader<TestAsset> bLobLoader(vts, sourceSection);
        for (const auto& it : bLobLoader.Assets())
        {
            io::Tag copiedTag = vts.CopyTag(it->mTag, targetSection, Options::Flat);
            io::Buffer data = io::CloneBuffer(it->mBuffer);
            assets.push_back(std::make_shared<TestAsset>(copiedTag, data, vts));
        }

        CHECK(vts.AttachBlob(assets));
        CHECK_EQUAL(bLobLoader.Assets().size(), vts.GetNumTags(targetSection));
    }

    //--------------------------------------------------------------------------------------------------
    // cleanup everything
    {
        io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        CHECK(vts.DeleteBlob({ sourceSection, targetSection }));
        CHECK_EQUAL(0, vts.GetNumTags({ sourceSection, targetSection }));
    }

    //--------------------------------------------------------------------------------------------------
    const Section file1(sourceSection.ToString() + "/File.txt");
    const Section file2(sourceSection.ToString() + "/File2.txt");
    const Section file3(sourceSection.ToString() + "/File_foo.txt");
    const Section file4(sourceSection.ToString() + "/Varoom.txt");
    // create files for search/find test
    {
        io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        io::Tag copyTag = vts.GenerateTag(file1);
        io::Buffer data = io::CreateBuffer("xxx");
        CHECK(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file2);
        data = io::CreateBuffer("xxx");
        CHECK(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file3);
        data = io::CreateBuffer("xxx");
        CHECK(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));

        copyTag = vts.GenerateTag(file4);
        data = io::CreateBuffer("xxx");
        CHECK(vts.AttachBlob(std::make_shared<TestAsset>(copyTag, data, vts)));
    }

    //--------------------------------------------------------------------------------------------------
    // test finding partial match and exact match
    {
        io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        Section finder(sourceSection.ToString() + "/File");
        CHECK_EQUAL(3, vts.GetNumTags(finder));

        finder = Section(sourceSection.ToString() + "/Varoom");
        CHECK_EQUAL(1, vts.GetNumTags(finder));

        finder = Section(sourceSection.ToString() + "/File3");
        CHECK_EQUAL(0, vts.GetNumTags(finder));
    }

    //--------------------------------------------------------------------------------------------------
    const Section seting("WriteTestSettings");
    // create files to test override
    {
        io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        CHECK(vts.DeleteBlob(seting));
        CHECK_EQUAL(0, vts.GetNumTags(seting));

        const Section settingsFile1("WriteTestSettings@Settings/Bindings.json");
        io::Tag tag1 = vts.GenerateTag(settingsFile1);
        io::Buffer data1 = io::CreateBuffer("Exit App");
        CHECK(vts.AttachBlob(std::make_shared<TestAsset>(tag1, data1, vts)));

        const Section settingsFile2("WriteTestSettings@User/Settings/Bindings.json");
        io::Tag tag2 = vts.GenerateTag(settingsFile2);
        io::Buffer data2 = io::CreateBuffer("Sound Options");
        CHECK(vts.AttachBlob(std::make_shared<TestAsset>(tag2, data2, vts)));
    }

    //--------------------------------------------------------------------------------------------------
    // get override settings file
    {
        io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        const Section settingsFile(">TestSettings@Bindings");

        io::BLobLoader<TestAsset> settingLoader(vts, settingsFile);
        CHECK_EQUAL(1, settingLoader.Assets().size());
        auto settingAsset = *settingLoader.Assets().begin();
        CHECK_EQUAL("Sound Options", settingAsset->mMessage);

        const Section settingsFiles("TestSettings@Bindings");

        io::BLobLoader<TestAsset> settingsLoader(vts, settingsFiles);
        CHECK_EQUAL(2, settingsLoader.Assets().size());
        auto asset0 = settingsLoader.Assets()[0];
        CHECK_EQUAL("Sound Options", asset0->mMessage);
        auto asset1 = settingsLoader.Assets()[1];
        CHECK_EQUAL("Exit App", asset1->mMessage);

        {
            std::vector<io::Tag> tags = vts.GetTags(settingsFile);
            CHECK_EQUAL(1, tags.size());
            io::Tag cachedTag = *tags.begin();

            io::Buffer buffer = io::CreateBuffer("Cached Sound Options");
            auto cachedAsset = std::make_shared<TestAsset>(cachedTag, buffer, vts);
            vts.AddOverride(cachedAsset);
        }

        io::BLobLoader<TestAsset> cachedLoader(vts, settingsFile);
        CHECK_EQUAL(1, cachedLoader.Assets().size());
        auto cachedAsset = *cachedLoader.Assets().begin();
        CHECK_EQUAL("Cached Sound Options", cachedAsset->mMessage);
    }

    //--------------------------------------------------------------------------------------------------
    // cleanup settings folder
    {
        io::tool::VirtualTransportSystemDefault vts(dev::CurrentConfiguration().mInit.mVTSConfig, Resolvers, vtsFile);

        CHECK(vts.DeleteBlob({ seting, sourceSection }));
        CHECK_EQUAL(0, vts.GetNumTags({ seting, sourceSection }));
    }

#endif
}
