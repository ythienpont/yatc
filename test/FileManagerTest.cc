#include "FileManager/FileManager.h"
#include <gtest/gtest.h>
#include <stdexcept>

class LinuxFileManagerTest : public ::testing::Test {
protected:
  LinuxFileManager *manager;
  std::vector<FileInfo> files;
  size_t pieceLength;

  void SetUp() override {
    pieceLength = 512; // Assume each piece is 512 bytes
    files.push_back({"file1.txt", 1024, 0, 1023}); // A file of 1024 bytes
    manager = new LinuxFileManager(files, pieceLength);
  }

  void TearDown() override { delete manager; }
};

// Test writing a block within the piece boundary
TEST_F(LinuxFileManagerTest, WriteBlockWithinBounds) {
  BlockInfo block{0, pieceLength / 2}; // Write half piece
  std::vector<char> data(pieceLength / 2, 'A');
  ASSERT_TRUE(manager->writeBlock(0, block, data));
}

// Test writing a block out of bounds
TEST_F(LinuxFileManagerTest, WriteBlockOutOfBounds) {
  BlockInfo block{0, pieceLength * 2}; // Attempt to write double the piece size
  std::vector<char> data(pieceLength * 2, 'B');
  EXPECT_THROW(manager->writeBlock(0, block, data), std::out_of_range);
}
/*
// Test that the piece is marked complete when all blocks have been written
TEST_F(LinuxFileManagerTest, WriteCompletePiece) {
  size_t blockSize = pieceLength / 4; // This divides the piece into 4 blocks

  // Write the piece in 4 parts
  for (int i = 0; i < 2; ++i) {
    BlockInfo block{i * blockSize, blockSize};
    std::vector<char> data(blockSize, 'C' + i); // Different data for each block
    ASSERT_TRUE(manager->writeBlock(0, block, data));
  }
}
*/
