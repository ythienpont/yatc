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

  // The length of the file, in bytes.
  uint64_t length;

  // Return if the download represents a single file
  inline bool isSingleFile() const;

  Torrent() = default;
  ~Torrent() = default;
};

#endif // TORRENT_H
