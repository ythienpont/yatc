#ifndef PIECEMANAGER_H
#define PIECEMANAGER_H

#include <Torrent/Torrent.h>
#include <cstdint>
#include <mutex>
#include <unordered_set>
#include <vector>

/**
 * @brief Manages the pieces of a torrent download.
 *
 * This class keeps track of which pieces have been downloaded and which are
 * still missing.
 */
class PieceManager {
public:
  /**
   * @brief Constructs a PieceManager with the total size and piece length of
   * the torrent.
   *
   * @param total_size The total size of the torrent in bytes.
   * @param piece_length The length of each piece in bytes.
   */
  PieceManager(const uint64_t total_size, const uint32_t piece_length);

  /**
   * @brief Checks if a piece has been downloaded.
   *
   * @param piece_index The index of the piece to check.
   * @return true if the piece has been downloaded, false otherwise.
   */
  bool has_piece(const uint32_t piece_index) const;

  /**
   * @brief Marks a piece as downloaded.
   *
   * @param piece_index The index of the piece to mark as downloaded.
   */
  void save_piece(const uint32_t piece_index);

  /**
   * @brief Retrieves a set of all missing pieces.
   *
   * @return A set containing the indices of all missing pieces.
   */
  std::unordered_set<uint32_t> missing_pieces() const;

  /**
   * @brief Gets the size of a specific piece.
   *
   * @param piece_index The index of the piece.
   * @return The size of the piece in bytes.
   */
  uint32_t piece_size(const uint32_t piece_index);

private:
  mutable std::mutex
      mutex_; ///< Mutex for thread-safe access to member variables.
  std::vector<bool> downloaded_pieces_; ///< Vector tracking the download status
                                        ///< of each piece.
  uint64_t total_size;                  ///< Total size of the torrent in bytes.
  uint32_t piece_length;                ///< Length of each piece in bytes.
  uint32_t total_pieces; ///< Total number of pieces in the torrent.
};

#endif // PIECEMANAGER_H
