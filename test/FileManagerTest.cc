#include "FileManager/FileManager.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class FileManagerTest : public ::testing::Test {
protected:
  std::vector<FileInfo> mockFiles;
  uint32_t pieceLength = 1024; // Example piece length
  std::vector<InfoHash>
      infoHashes; // You would need to mock or define infoHashes appropriate for
                  // your testing environment

  virtual void SetUp() {
    mockFiles.push_back(
        FileInfo{"test/test_files/file1.bin", 2048, 0, 2048}); // Example file
    for (size_t i = 0; i < 2; ++i) {
      infoHashes.push_back(
          {}); // Assuming InfoHash can be default constructed or mocked
    }
  }
};

TEST_F(FileManagerTest, InitializesCorrectly) {
  ASSERT_NO_THROW(LinuxFileManager(mockFiles, pieceLength, infoHashes));
}

TEST_F(FileManagerTest, ThrowsOnHashPieceMismatch) {
  infoHashes.pop_back(); // Remove one hash to create mismatch
  ASSERT_THROW(LinuxFileManager(mockFiles, pieceLength, infoHashes),
               std::invalid_argument);
}

TEST_F(FileManagerTest, WriteAndReadCompletePiece) {
  LinuxFileManager manager(mockFiles, pieceLength, infoHashes);
  std::vector<char> testData(pieceLength,
                             'x'); // Fill piece with 'x' to simulate full piece

  // Write enough blocks to cover the entire piece
  bool writeSuccess = manager.writeBlock(0, 0, testData);
  ASSERT_TRUE(writeSuccess);

  // Optional: Confirm that the piece is marked as complete if your design
  // allows for this check ASSERT_TRUE(manager.isPieceComplete(0)); // This
  // requires you exposing such functionality

  // Now read back the data
  auto readData = manager.readBlock(0, 0, pieceLength);
  ASSERT_EQ(readData.size(), testData.size());
  EXPECT_THAT(
      readData,
      ::testing::ElementsAreArray(
          testData)); // Using ElementsAreArray to compare the whole vector
}

TEST_F(FileManagerTest, WriteBlockOutOfBounds) {
  LinuxFileManager manager(mockFiles, pieceLength, infoHashes);
  std::vector<char> testData = {'x', 'y', 'z'};
  EXPECT_FALSE(
      manager.writeBlock(5, 0, testData)); // Assuming 5 is out of range
}

TEST_F(FileManagerTest, WritesCompletePiece) {
  LinuxFileManager manager(mockFiles, pieceLength, infoHashes);
  std::vector<char> testData(pieceLength, 'x');    // Data that fills a piece
  EXPECT_TRUE(manager.writeBlock(0, 0, testData)); // Write a full piece
  // Further expectations could include checking file contents or mocking file
  // I/O
}
