#ifndef PIECEMANAGER_H
#define PIECEMANAGER_H

#include <Torrent/Torrent.h>
#include <unordered_set>
#include <vector>

/**
 * @brief Manages the status of pieces in a torrent download.
 *
 * Tracks which pieces have been successfully downloaded and which are still
 * missing.
 */
class PieceManager {
public:
  /**
   * @brief Constructs a new Piece Manager object.
   *
   * @param totalPieces Total number of pieces in the torrent.
   */
  PieceManager(const size_t totalPieces);

  /**
   * @brief Checks if a piece has already been downloaded.
   *
   * @param pieceIndex Index of the piece to check.
   * @return true If the piece is downloaded.
   * @return false If the piece is not downloaded.
   */
  bool hasPiece(const size_t pieceIndex) const;

  /**
   * @brief Marks a piece as downloaded.
   *
   * @param pieceIndex Index of the piece to mark as downloaded.
   */
  void savePiece(const size_t pieceIndex);

  /**
   * @brief Retrieves a set of indices representing all pieces that are still
   * missing.
   *
   * @return std::unordered_set<size_t> Set of missing piece indices.
   */
  std::unordered_set<size_t> missingPieces() const;

private:
  /// @brief Tracks the download status of each piece.
  std::vector<bool> downloadedPieces_;
};

#endif // PIECEMANAGER_H
