#include "FileManager/FileManager.h"
#include <fstream>
#include <gtest/gtest.h>
#include <stdexcept>

class LinuxFileManagerTest : public ::testing::Test {
protected:
  LinuxFileManager *manager;
  std::vector<FileInfo> files;
  size_t pieceLength;

  void SetUp() override {
    pieceLength = 512; // Assume each piece is 512 bytes
    files.push_back(
        {"file1.txt", 1024, 0, 1023}); // A file of 1024 bytes, 2 pieces
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
  ASSERT_FALSE(manager->writeBlock(0, block, data));
}

// Test that the piece is marked complete when all blocks have been written
TEST_F(LinuxFileManagerTest, WriteCompletePiece) {
  size_t blockSize = pieceLength / 2;

  for (int i = 0; i < 2; ++i) {
    BlockInfo block{i * blockSize, blockSize};
    std::vector<char> data(blockSize, 'A' + i); // Different data for each block
    ASSERT_TRUE(manager->writeBlock(0, block, data));
  }

  std::ifstream file("file1.txt");
  ASSERT_TRUE(file);

  constexpr size_t bytesToRead = 512;
  std::vector<char> buffer(bytesToRead);

  file.read(buffer.data(), bytesToRead);
  std::streamsize bytes_read = file.gcount();

  ASSERT_TRUE(bytes_read == 512);

  for (size_t i = 0; i < 256; ++i) {
    ASSERT_EQ(buffer[i], 'A'); // First half should be 'A'
  }
  for (size_t i = 256; i < 512; ++i) {
    ASSERT_EQ(buffer[i], 'B'); // Second half should be 'B'
  }
}

TEST_F(LinuxFileManagerTest, WriteReadBlockRoundtrip) {
  size_t blockSize = pieceLength / 2;

  for (int i = 0; i < 2; ++i) {
    BlockInfo block{i * blockSize, blockSize};
    std::vector<char> data(blockSize, 'A' + i); // Different data for each block
    ASSERT_TRUE(manager->writeBlock(0, block, data));
  }
  std::vector<char> aBuffer = manager->readBlock(0, {0, blockSize});
  std::vector<char> bBuffer = manager->readBlock(0, {blockSize, blockSize});

  for (size_t i = 0; i < 256; ++i) {
    ASSERT_EQ(aBuffer[i], 'A'); // First half should be 'A'
    ASSERT_EQ(bBuffer[i], 'B'); // Second half should be 'B'
  }
}

TEST_F(LinuxFileManagerTest, WriteReadPieceRoundtrip) {
  size_t blockSize = pieceLength / 2;

  for (int i = 0; i < 2; ++i) {
    BlockInfo block{i * blockSize, blockSize};
    std::vector<char> data(blockSize, 'A' + i); // Different data for each block
    ASSERT_TRUE(manager->writeBlock(0, block, data));
  }

  std::vector<char> buffer = manager->readPiece(0);

  for (size_t i = 0; i < 256; ++i) {
    ASSERT_EQ(buffer[i], 'A'); // First half should be 'A'
  }

  for (size_t i = 256; i < 512; ++i) {
    ASSERT_EQ(buffer[i], 'B'); // Second half should be 'B'
  }
}
