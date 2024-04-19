#ifndef PIECEMANAGER_H
#define PIECEMANAGER_H

#include <Torrent/Torrent.h>
#include <unordered_set>
#include <vector>

/*
 * Manages the status of pieces in a torrent download, tracking which pieces
 * have been successfully downloaded and which are still missing.
 */
class PieceManager {
public:
  PieceManager(const size_t totalPieces);

  // Checks if a piece has already been downloaded.
  bool hasPiece(const size_t pieceIndex) const;

  // Marks the piece as downloaded
  void savePiece(const size_t pieceIndex);

  // Retrieves a set of indices representing all pieces that are still missing.
  std::unordered_set<size_t> missingPieces() const;

private:
  // Tracks the download status of each piece.
  std::vector<bool> downloadedPieces_;
};

#endif // PIECEMANAGER_H
