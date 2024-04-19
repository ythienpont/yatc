#include "PieceManager.h"
#include <openssl/sha.h>
#include <stdexcept>

PieceManager::PieceManager(const size_t totalPieces) {
  downloadedPieces_.resize(totalPieces, false);
}

bool PieceManager::hasPiece(size_t pieceIndex) const {
  return pieceIndex < downloadedPieces_.size() && downloadedPieces_[pieceIndex];
}

std::unordered_set<size_t> PieceManager::missingPieces() const {
  std::unordered_set<size_t> missing;
  for (size_t i = 0; i < downloadedPieces_.size(); ++i) {
    if (!downloadedPieces_[i]) {
      missing.insert(i);
    }
  }
  return missing;
}

void PieceManager::savePiece(size_t pieceIndex) {
  if (pieceIndex >= downloadedPieces_.size()) {
    throw std::out_of_range("Attempted to access an invalid piece index");
  }

  if (!downloadedPieces_[pieceIndex]) {
    downloadedPieces_[pieceIndex] = true;
  }
}
