#ifndef TORRENT_H
#define TORRENT_H

#include <array>
#include <bencode.hpp>
#include <cstdint>
#include <stddef.h>
#include <string>
#include <vector>

/**
 * @struct FileInfo
 * @brief Holds metadata about a single file within a torrent.
 *
 * This includes the file's path, its total length, and byte offsets indicating
 * where the file starts and ends within the overall content of the torrent.
 */
struct FileInfo {
  std::string path;      ///< Full path of the file.
  uint64_t length;       ///< Total length of the file in bytes.
  uint64_t start_offset; ///< Byte index where the file starts.
  uint64_t end_offset;   ///< Byte index where the file ends.
};

/**
 * @typedef InfoHash
 * @brief Represents the SHA-1 hash used to identify the torrent.
 */
using InfoHash = std::array<std::byte, 20>;

/**
 * @struct Torrent
 * @brief Represents the essential information of a torrent.
 *
 * Includes the torrent's metadata such as the name, info dictionary from the
 * bencoded file, tracker URL, piece length, file information, and hashes for
 * each piece of the torrent.
 */
struct Torrent {
  std::string name; ///< Name of the torrent.
  bencode::dict
      info; ///< Decoded bencode dictionary containing the torrent's metadata.
  std::string tracker_url; ///< URL of the tracker server for announcements.
  InfoHash info_hash =
      {}; ///< The 20-byte SHA-1 hash of the bencoded 'info' value.
  uint32_t piece_length;       ///< Number of bytes each piece contains.
  std::vector<FileInfo> files; ///< List of files included in the torrent.
  std::vector<InfoHash>
      pieces; ///< List of SHA-1 hashes for each piece of the torrent.

  /**
   * @brief Checks if the torrent represents a single file.
   *
   * @return true if the torrent represents a single file, false otherwise.
   */
  bool is_single_file() const;

  /**
   * @brief Returns the total number of pieces in the torrent.
   *
   * @return size_t The total number of pieces.
   */
  uint32_t total_pieces() const;

  /**
   * @brief Returns the total size of the torrent.
   *
   * @return uint64_t The total size of the torrent.
   */
  uint64_t size() const;

  /**
   * @brief Default constructor for Torrent.
   */
  Torrent() = default;

  /**
   * @brief Default destructor for Torrent.
   */
  ~Torrent() = default;

  /**
   * @brief Provides diagnostic information for debugging.
   *
   * @return std::string Diagnostic information.
   */
  std::string diagnostic_info() const;
};

#endif // TORRENT_H
