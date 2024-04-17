#ifndef TORRENT_H
#define TORRENT_H

#include <array>
#include <bencode.hpp>
#include <cstdint>
#include <string>
#include <vector>

struct FileInfo {
  std::string path;
  uint64_t length;
  size_t startOffset; // Index to first byte of the file
  size_t endOffset;   // Index to last byte of the file
};

using InfoHash = std::array<std::byte, 20>;

struct Torrent {
  // Name of the torrent file
  std::string name;

  bencode::dict info;

  // The URL of the tracker server to which announce requests are sent.
  std::string trackerUrl;

  // The 20 byte sha1 hash of the bencoded form of the info value from the
  // metainfo file.
  InfoHash infoHash = {};

  // Number of bytes in each piece the file is split into
  uint64_t pieceLength;

  std::vector<FileInfo> files;

  // The SHA1 hash of the piece at the corresponding index
  std::vector<InfoHash> pieces;

  // Return if the download represents a single file
  inline bool isSingleFile() const;

  // Return the total file size of the torrent
  size_t totalFileSize() const;

  // Return the total amount of pieces
  size_t totalPieces() const;

  Torrent() = default;
  ~Torrent() = default;
};

#endif // TORRENT_H
