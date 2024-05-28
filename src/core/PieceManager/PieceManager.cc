#include "PieceManager.h"
#include <stdexcept>

PieceManager::PieceManager(const uint32_t total_pieces) {
  downloaded_pieces_.resize(total_pieces, false);
}

bool PieceManager::has_piece(const uint32_t piece_index) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return piece_index < downloaded_pieces_.size() &&
         downloaded_pieces_[piece_index];
}

std::unordered_set<uint32_t> PieceManager::missing_pieces() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::unordered_set<uint32_t> missing;
  for (uint32_t i = 0; i < downloaded_pieces_.size(); ++i) {
    if (!downloaded_pieces_[i]) {
      missing.insert(i);
    }
  }
  return missing;
}

void PieceManager::save_piece(const uint32_t piece_index) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (piece_index >= downloaded_pieces_.size()) {
    throw std::out_of_range("Attempted to access an invalid piece index");
  }

  if (!downloaded_pieces_[piece_index]) {
    downloaded_pieces_[piece_index] = true;
  }
}
