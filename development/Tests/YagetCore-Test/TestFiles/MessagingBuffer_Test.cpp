#include "pch.h"
#include "Streams/Buffers.h"
#include "TestHelpers/TestHelpers.h"

//CHECK_EQUAL(expected, actual);
class MessageBuffer : public ::testing::Test
{
private:
    yaget::test::Environment mEnvironment;
};


TEST_F(MessageBuffer, DefaultCtor)
{
    using namespace yaget::io;
    
    MessagingBuffer buf;
    EXPECT_EQ(0u, size_data(buf.mBuffer));
    EXPECT_EQ(0u, buf.mWriteOffset);
    EXPECT_EQ(0u, buf.mNumEntities);
}


TEST_F(MessageBuffer, AssureWriteSize_ResizesWhenNeeded)
{
    using namespace yaget::io;
    
    // start with small buffer to force reallocation
    MessagingBuffer buf(4);
    EXPECT_EQ(4u, size_data(buf.mBuffer));

    // request additional size that doesn't fit current capacity
    buf.AssureWriteSize(5);
    // nextBufferSize = (currentCapacity * 2) + additionalSize -> (4*2)+5 = 13
    EXPECT_EQ(13u, size_data(buf.mBuffer));
}


TEST_F(MessageBuffer, WriteDataChunk_PointerOverload_And_Shrink)
{
    using namespace yaget::io;
    
    MessagingBuffer buf;
    const char sample[] = "hello";
    const size_t len = 5;

    // ensure buffer has enough capacity
    buf.AssureWriteSize(len);
    EXPECT_TRUE(size_data(buf.mBuffer) >= len);

    // write using pointer overload
    buf.WriteDataChunk(sample, len);
    EXPECT_EQ(len, buf.mWriteOffset);
    EXPECT_TRUE(std::memcmp(cast_data<const char>(buf.mBuffer), sample, len) == 0);

    // Shrink should set buffer size to write offset
    buf.Shrink();
    EXPECT_EQ(buf.mWriteOffset, size_data(buf.mBuffer));
}


TEST_F(MessageBuffer, WriteDataChunk_BufferOverload)
{
    using namespace yaget::io;
    
    const char world[] = "world";
    const size_t len = 5;

    // create source buffer
    Buffer src = CreateBuffer(reinterpret_cast<const uint8_t*>(world), len);

    // messaging buffer with enough initial capacity
    MessagingBuffer buf(10);
    EXPECT_EQ(10u, size_data(buf.mBuffer));

    // write using Buffer overload
    buf.WriteDataChunk(src);
    EXPECT_EQ(len, buf.mWriteOffset);
    EXPECT_TRUE(std::memcmp(cast_data<const char>(buf.mBuffer), world, len) == 0);
}

#if 0
#include "YagetCore.h"
#include "Streams/Buffers.h"
#include "UnitTest++.h"

#include <cstring>
#include <string>

using namespace yaget::io;

TEST(MessagingBuffer_DefaultCtor)
{
    MessagingBuffer buf;
    EXPECT_EQ(0u, size_data(buf.mBuffer));
    EXPECT_EQ(0u, buf.mWriteOffset);
    EXPECT_EQ(0u, buf.mNumEntities);
}

TEST(MessagingBuffer_AssureWriteSize_ResizesWhenNeeded)
{
    // start with small buffer to force reallocation
    MessagingBuffer buf(4);
    CHECK_EQUAL(4u, size_data(buf.mBuffer));

    // request additional size that doesn't fit current capacity
    buf.AssureWriteSize(5);
    // nextBufferSize = (currentCapacity * 2) + additionalSize -> (4*2)+5 = 13
    CHECK_EQUAL(13u, size_data(buf.mBuffer));
}

TEST(MessagingBuffer_WriteDataChunk_PointerOverload_And_Shrink)
{
    MessagingBuffer buf;
    const char sample[] = "hello";
    const size_t len = 5;

    // ensure buffer has enough capacity
    buf.AssureWriteSize(len);
    CHECK(size_data(buf.mBuffer) >= len);

    // write using pointer overload
    buf.WriteDataChunk(sample, len);
    CHECK_EQUAL(len, buf.mWriteOffset);
    CHECK(std::memcmp(cast_data<const char>(buf.mBuffer), sample, len) == 0);

    // Shrink should set buffer size to write offset
    buf.Shrink();
    CHECK_EQUAL(buf.mWriteOffset, size_data(buf.mBuffer));
}

TEST(MessagingBuffer_WriteDataChunk_BufferOverload)
{
    const char world[] = "world";
    const size_t len = 5;

    // create source buffer
    Buffer src = CreateBuffer(reinterpret_cast<const uint8_t*>(world), len);

    // messaging buffer with enough initial capacity
    MessagingBuffer buf(10);
    CHECK_EQUAL(10u, size_data(buf.mBuffer));

    // write using Buffer overload
    buf.WriteDataChunk(src);
    CHECK_EQUAL(len, buf.mWriteOffset);
    CHECK(std::memcmp(cast_data<const char>(buf.mBuffer), world, len) == 0);
}
#endif