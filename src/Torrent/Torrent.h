#ifndef TORRENT_H
#define TORRENT_H

#include "../../lib/bencode.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct FileInfo {
  std::string path;
  uint64_t length;
};
struct Torrent {
  // Name of the torrent file
  std::string name;

  bencode::dict info;

  // The URL of the tracker server to which announce requests are sent.
  std::string trackerUrl;

  // The 20 byte sha1 hash of the bencoded form of the info value from the
  // metainfo file.
  std::array<char, 20> infoHash = {};

  // Number of bytes in each piece the file is split into
  uint64_t pieceLength;

  std::vector<FileInfo> files;

  // The SHA1 hash of the piece at the corresponding index
  std::vector<std::array<char, 20>> pieces;

  // The length of the file, in bytes.
  uint64_t length;

  // Return if the download represents a single file
  inline bool isSingleFile() const;

  Torrent();
  Torrent(const std::string &name, const std::string &trackerUrl,
          const std::array<char, 20> &infoHash);

  ~Torrent() = default;
};

#endif // TORRENT_H
