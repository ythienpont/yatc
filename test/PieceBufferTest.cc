#include "PieceBuffer/PieceBuffer.h"
#include <gtest/gtest.h>

class PieceBufferInfoTest : public ::testing::Test {
protected:
  PieceBufferInfo *bufferInfo;

  void SetUp() override {
    bufferInfo = new PieceBufferInfo(
        1024, {}); // Assuming InfoHash can be default constructed
  }

  void TearDown() override { delete bufferInfo; }
};

TEST_F(PieceBufferInfoTest, InitializesCorrectly) {
  EXPECT_EQ(bufferInfo->isComplete(), false);
  EXPECT_EQ(bufferInfo->isActive(), false);
}

TEST_F(PieceBufferInfoTest, AddsBlocksCorrectly) {
  ASSERT_TRUE(bufferInfo->addBlock(0, 256)); // First block
  EXPECT_EQ(bufferInfo->isActive(), true);

  ASSERT_NO_THROW(bufferInfo->addBlock(256, 256)); // Second block
  ASSERT_THROW(bufferInfo->addBlock(2048, 256),
               std::runtime_error); // Out of range block
  ASSERT_THROW(bufferInfo->addBlock(128, 256),
               std::runtime_error); // Misaligned block
}

TEST_F(PieceBufferInfoTest, HandlesCompleteAndActiveStatus) {
  bufferInfo->addBlock(0, 256);
  bufferInfo->addBlock(256, 256);
  bufferInfo->addBlock(512, 256);
  bufferInfo->addBlock(768, 256);
  EXPECT_EQ(bufferInfo->isComplete(), true);
  EXPECT_EQ(bufferInfo->isActive(), true);
}

TEST_F(PieceBufferInfoTest, ClearsCorrectly) {
  bufferInfo->addBlock(0, 256);
  bufferInfo->clear();
  EXPECT_EQ(bufferInfo->isActive(), false);
  EXPECT_EQ(bufferInfo->isComplete(), false);
}

TEST(PieceBuffer, HandlesDataCorrectly) {
  PieceBuffer buffer(1024);
  std::vector<char> data = {'a', 'b', 'c', 'd'};

  ASSERT_NO_THROW(buffer.addData(0, data));
  ASSERT_THROW(buffer.addData(1022, data),
               std::runtime_error); // Should throw due to overflow

  const std::vector<char> &retData = buffer.getData();
  EXPECT_EQ(retData[0], 'a');
  EXPECT_EQ(retData[1], 'b');
}
