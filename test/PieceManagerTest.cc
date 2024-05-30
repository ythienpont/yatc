#include "PieceManager/PieceManager.h"
#include <gtest/gtest.h>
#include <unordered_set>

class PieceManagerTest : public ::testing::Test {
protected:
  uint64_t total_size;
  uint32_t piece_length;
  PieceManager *pm;

  void SetUp() override {
    total_size = 1000;
    piece_length = 100;
    pm = new PieceManager(total_size, piece_length);
  }

  void TearDown() override { delete pm; }
};

TEST_F(PieceManagerTest, Initialization) {
  EXPECT_EQ(pm->piece_size(0), 100);
  EXPECT_EQ(pm->piece_size(9), 100);
  EXPECT_EQ(pm->piece_size(10), 0); // Out of bounds
}

TEST_F(PieceManagerTest, HasPiece) {
  EXPECT_FALSE(pm->has_piece(0));
  pm->save_piece(0);
  EXPECT_TRUE(pm->has_piece(0));
}

TEST_F(PieceManagerTest, SavePiece) {
  pm->save_piece(5);
  EXPECT_TRUE(pm->has_piece(5));
  EXPECT_FALSE(pm->has_piece(4));
}

TEST_F(PieceManagerTest, MissingPieces) {
  std::unordered_set<uint32_t> expected_missing = {0, 1, 2, 3, 4,
                                                   5, 6, 7, 8, 9};
  EXPECT_EQ(pm->missing_pieces(), expected_missing);

  pm->save_piece(5);
  expected_missing.erase(5);
  EXPECT_EQ(pm->missing_pieces(), expected_missing);
}

TEST_F(PieceManagerTest, PieceSize) {
  PieceManager pm_large(1050, 100);
  EXPECT_EQ(pm_large.piece_size(0), 100);
  EXPECT_EQ(pm_large.piece_size(10), 50); // Last piece is smaller
  EXPECT_EQ(pm_large.piece_size(11), 0);  // Out of bounds
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
