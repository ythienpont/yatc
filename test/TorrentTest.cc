#include "Torrent/Torrent.h"
#include <gtest/gtest.h>

class TorrentTest : public ::testing::Test {
protected:
  Torrent torrent;

  void SetUp() override {
    // Common setup code here, if any
  }

  void TearDown() override {
    // Cleanup code here, if any
  }
};

TEST_F(TorrentTest, IsSingleFileTrue) {
  torrent.files.push_back(FileInfo{"singlefile.mp4", 1000000, 0, 999999});
  ASSERT_TRUE(torrent.isSingleFile());
}

TEST_F(TorrentTest, IsSingleFileFalseWithMultipleFiles) {
  torrent.files.push_back(FileInfo{"part1.mp4", 500000, 0, 499999});
  torrent.files.push_back(FileInfo{"part2.mp4", 500000, 500000, 999999});
  ASSERT_FALSE(torrent.isSingleFile());
}

TEST_F(TorrentTest, IsSingleFileFalseWithNoFiles) {
  ASSERT_FALSE(torrent.isSingleFile());
}

TEST_F(TorrentTest, TotalPiecesNoPieces) {
  ASSERT_EQ(0, torrent.totalPieces());
}

TEST_F(TorrentTest, TotalPiecesOnePiece) {
  torrent.pieces.push_back({});
  ASSERT_EQ(1, torrent.totalPieces());
}

TEST_F(TorrentTest, TotalPiecesMultiplePieces) {
  torrent.pieces.push_back({});
  torrent.pieces.push_back({});
  torrent.pieces.push_back({});
  ASSERT_EQ(3, torrent.totalPieces());
}
