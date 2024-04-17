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

/*
bool PieceManager::checkIntegrity(size_t pieceIndex,
                                  const std::string &expectedHash) {
  // This is a simplified placeholder for actual hash checking logic
  return calculatePieceHash(std::vector<char>()) == expectedHash;
}


std::string
PieceManager::calculatePieceHash(const std::vector<char> &data) const {
  // Placeholder for hash calculation
  unsigned char hash[SHA_DIGEST_LENGTH];
  SHA1(reinterpret_cast<const unsigned char *>(data.data()), data.size(), hash);
  std::stringstream ss;
  for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
  }
  return ss.str();
}
*/
