#include "pch.h" 
#include "Compression/Zipper.h"
#include "Debugging/DevConfiguration.h"
#include "TestHelpers/TestHelpers.h"
#include "Json/JsonHelpers.h"


//CHECK_EQUAL(expected, actual);
class ZipUnzip : public ::testing::Test
{
};


TEST_F(ZipUnzip, ZipUnzip_RoundTrip_StringData)
{
    using namespace yaget;

    const std::string original = "The quick brown fox jumps over the lazy dog";
    auto srcBuffer = io::CreateBuffer(original);

    auto zipped = compression::ZipBuffer(io::cast_to_view(srcBuffer));
    // Ensure something was produced
    EXPECT_GT(io::size_data(zipped), 0u);

    auto unzipped = compression::UnzipBuffer(io::cast_to_view(zipped));
    // Unzipped should match original
    EXPECT_EQ(io::size_data(unzipped), original.size());
    auto resultCmp = unzipped == srcBuffer;
    resultCmp;
    EXPECT_TRUE(unzipped == srcBuffer);
}


TEST_F(ZipUnzip, Unzip_InvalidSignature_ReturnsEmpty)
{
    using namespace yaget;

    // Create a buffer that is too small to contain a valid signature
    const std::string garbage = "not-a-zip";
    auto garbageBuffer = io::CreateBuffer(garbage);

    auto result = compression::UnzipBuffer(io::cast_to_view(garbageBuffer));
    // Expect an empty buffer (size == 0)
    EXPECT_EQ(io::size_data(result), 0u);
}


TEST_F(ZipUnzip, ZipUnzip_EmptyBuffer)
{
    using namespace yaget;

    // Create an empty buffer
    auto empty = io::CreateBuffer(static_cast<size_t>(0));
    auto zipped = compression::ZipBuffer(io::cast_to_view(empty));

    // Depending on implementation zipped buffer may still contain header; ensure unzip handles it
    EXPECT_GE(io::size_data(zipped), 0u);

    auto unzipped = compression::UnzipBuffer(io::cast_to_view(zipped));
    // Unzipping an empty payload should produce an empty buffer
    EXPECT_EQ(io::size_data(unzipped), 0u);
}
