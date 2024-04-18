#include "PieceBuffer/PieceBuffer.h"
#include <gtest/gtest.h>

class PieceBufferTest : public ::testing::Test {
protected:
  PieceBuffer *buffer;

  void SetUp() override {
    // Assume pieceLength of 100 for simplicity in tests
    buffer = new PieceBuffer(100);
  }

  void TearDown() override { delete buffer; }
};

TEST_F(PieceBufferTest, AddBlockWithinBounds) {
  BlockInfo block(0, 20);
  std::vector<char> data(20, 'A');
  EXPECT_TRUE(buffer->addBlock(block, data));
}

TEST_F(PieceBufferTest, AddBlockOutOfBoundFails) {
  BlockInfo block(90, 20);
  std::vector<char> data(20, 'A');
  EXPECT_FALSE(buffer->addBlock(block, data));
}

TEST_F(PieceBufferTest, OverlappingBlocks) {
  BlockInfo block1(10, 20);
  BlockInfo block2(25, 30);
  std::vector<char> data1(20, 'A');
  std::vector<char> data2(30, 'B');
  buffer->addBlock(block1, data1);
  buffer->addBlock(block2, data2);
  EXPECT_FALSE(buffer->isComplete());
}

TEST_F(PieceBufferTest, NonContiguousBlocksNotComplete) {
  BlockInfo block1(0, 10);
  BlockInfo block2(20, 10);
  std::vector<char> data1(10, 'A');
  std::vector<char> data2(10, 'B');
  buffer->addBlock(block1, data1);
  buffer->addBlock(block2, data2);
  EXPECT_FALSE(buffer->isComplete());
}

TEST_F(PieceBufferTest, ContiguousBlocksComplete) {
  BlockInfo block1(0, 50);
  BlockInfo block2(50, 50);
  std::vector<char> data1(50, 'A');
  std::vector<char> data2(50, 'B');
  buffer->addBlock(block1, data1);
  buffer->addBlock(block2, data2);
  EXPECT_TRUE(buffer->isComplete());
}

TEST_F(PieceBufferTest, GetDataThrowsOnIncomplete) {
  BlockInfo block(0, 50);
  std::vector<char> data(50, 'A');
  buffer->addBlock(block, data);
  EXPECT_THROW(buffer->getData(), std::runtime_error);
}

TEST_F(PieceBufferTest, GetDataSuccessOnComplete) {
  BlockInfo block1(0, 50);
  BlockInfo block2(50, 50);
  std::vector<char> data1(50, 'A');
  std::vector<char> data2(50, 'B');
  buffer->addBlock(block1, data1);
  buffer->addBlock(block2, data2);
  EXPECT_NO_THROW({
    const auto &data = buffer->getData();
    ASSERT_EQ(data.size(), 100);
    // Optionally check content of data for further verification.
  });
}
