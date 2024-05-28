#ifndef PIECEMANAGER_H
#define PIECEMANAGER_H

#include <Torrent/Torrent.h>
#include <mutex>
#include <unordered_set>
#include <vector>

class PieceManager {
public:
  PieceManager(const uint32_t total_pieces);

  bool has_piece(const uint32_t piece_index) const;

  void save_piece(const uint32_t piece_index);

  std::unordered_set<uint32_t> missing_pieces() const;

private:
  mutable std::mutex mutex_;
  std::vector<bool> downloaded_pieces_;
};

#endif // PIECEMANAGER_H
