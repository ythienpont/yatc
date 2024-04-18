#include "PieceManager/PieceManager.h"
#include <gtest/gtest.h>

class PieceManagerTest : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(PieceManagerTest, InitializeCorrectly) {
  PieceManager pm(10);
  ASSERT_EQ(pm.missingPieces().size(), 10);
}

TEST_F(PieceManagerTest, MarkPieceAsDownloaded) {
  PieceManager pm(10);
  pm.savePiece(3);
  ASSERT_TRUE(pm.hasPiece(3));
}

TEST_F(PieceManagerTest, OutOfBoundsAccessThrows) {
  PieceManager pm(10);
  ASSERT_THROW(pm.savePiece(10), std::out_of_range);
}

TEST_F(PieceManagerTest, MissingPiecesUpdatesCorrectly) {
  PieceManager pm(10);
  pm.savePiece(0);
  pm.savePiece(1);
  pm.savePiece(9);
  auto missing = pm.missingPieces();
  ASSERT_EQ(missing.size(), 7);
  ASSERT_EQ(missing.count(0), 0);
  ASSERT_EQ(missing.count(1), 0);
  ASSERT_EQ(missing.count(9), 0);
  ASSERT_EQ(missing.count(2), 1);
}
