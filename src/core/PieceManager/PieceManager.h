#ifndef PIECEMANAGER_H
#define PIECEMANAGER_H

#include <Torrent/Torrent.h>
#include <cstdint>
#include <mutex>
#include <unordered_set>
#include <vector>

class PieceManager {
public:
  PieceManager(const uint64_t total_size, const uint32_t piece_length);

  bool has_piece(const uint32_t piece_index) const;

  void save_piece(const uint32_t piece_index);

  std::unordered_set<uint32_t> missing_pieces() const;

  uint32_t piece_size(const uint32_t piece_index);

private:
  mutable std::mutex mutex_;
  std::vector<bool> downloaded_pieces_;
  uint64_t total_size;
  uint32_t piece_length;
  uint32_t total_pieces;
};

#endif // PIECEMANAGER_H
