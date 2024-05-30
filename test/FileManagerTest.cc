#include "FileManager/FileManager.h"
#include <cstring>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

class LinuxFileManagerTest : public ::testing::Test {
protected:
  std::vector<FileInfo> files;
  uint32_t piece_length;
  std::vector<InfoHash> info_hashes;
  LinuxFileManager *lfm;

  void SetUp() override {
    files = {{"file1.txt", 500, 0, 500}, {"file2.txt", 500, 500, 1000}};
    piece_length = 100;
    info_hashes = std::vector<InfoHash>(10); // Initialize with dummy hashes

    lfm = new LinuxFileManager(files, piece_length, info_hashes);
  }

  void TearDown() override {
    delete lfm;
    // Clean up test files
    remove("file1.txt");
    remove("file2.txt");
  }
};

TEST_F(LinuxFileManagerTest, ReadBlock) {
  // Simulate a file with known content
  std::string test_data = "Test data for file read.";
  std::vector<std::byte> data(
      reinterpret_cast<const std::byte *>(test_data.data()),
      reinterpret_cast<const std::byte *>(test_data.data()) + test_data.size());

  int fd = open("file1.txt", O_WRONLY | O_CREAT, 0666);
  write(fd, test_data.data(), test_data.size());
  close(fd);

  auto buffer = lfm->read_block(0, 0, data.size());
  EXPECT_EQ(buffer.size(), data.size());
  EXPECT_TRUE(std::equal(buffer.begin(), buffer.end(), data.begin()));
}

TEST_F(LinuxFileManagerTest, WritePiece) {
  std::vector<std::byte> data(100, std::byte{0xAB});
  EXPECT_TRUE(lfm->write_piece(0, data));

  int fd = open("file1.txt", O_RDONLY);
  std::vector<std::byte> buffer(100);
  read(fd, buffer.data(), 100);
  close(fd);

  EXPECT_EQ(buffer, data);
}

TEST_F(LinuxFileManagerTest, PreAllocateSpace) {
  struct stat statbuf;
  EXPECT_EQ(stat("file1.txt", &statbuf), 0);
  EXPECT_EQ(statbuf.st_size, 500);
  EXPECT_EQ(stat("file2.txt", &statbuf), 0);
  EXPECT_EQ(statbuf.st_size, 500);
}
