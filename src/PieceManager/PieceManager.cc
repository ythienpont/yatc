#include "PieceManager.h"
#include <openssl/sha.h> // For SHA1 hash calculation
#include <iomanip>       // For std::setw and std::setfill
#include <sstream>       // For std::stringstream

PieceManager::PieceManager(const std::vector<std::string>& filePaths, size_t pieceSize)
    : filePaths(filePaths), pieceSize(pieceSize), totalPieces(calculateTotalPieces()) {
    downloadedPieces.resize(totalPieces, false);
}

bool PieceManager::hasPiece(size_t pieceIndex) const {
  return pieceIndex < downloadedPieces.size() && downloadedPieces[pieceIndex];
}

std::unordered_set<size_t> PieceManager::missingPieces() const {
  std::unordered_set<size_t> missing;
  for (size_t i = 0; i < downloadedPieces.size(); ++i) {
    if (!downloadedPieces[i]) {
      missing.insert(i);
    }
  }
  return missing;
}

bool PieceManager::savePiece(size_t pieceIndex, const std::vector<char>& data) {
  if (pieceIndex >= totalPieces) return false;
  // Assuming single file for simplicity
  std::ofstream file(filePaths.front(), std::ios::binary | std::ios::out | std::ios::in);
  if (!file) return false;

  file.seekp(pieceIndex * pieceSize);
  file.write(data.data(), data.size());
  if (file.fail()) return false;

  downloadedPieces[pieceIndex] = true;
  return true;
}

bool PieceManager::checkIntegrity(size_t pieceIndex, const std::string& expectedHash) {
    // This is a simplified placeholder for actual hash checking logic
    return calculatePieceHash(std::vector<char>()) == expectedHash;
}

size_t PieceManager::calculateTotalPieces() const {
    // Simplified calculation; should be based on actual file size
    return 0;
}

std::string PieceManager::calculatePieceHash(const std::vector<char>& data) const {
    // Placeholder for hash calculation
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.data()), data.size(), hash);
    std::stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}
