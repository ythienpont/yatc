#ifndef TORRENT_H
#define TORRENT_H

#include <string>
#include <cstdint>
#include <array>

class Torrent {
public:
  // The URL of the tracker server to which announce requests are sent.
  const std::string trackerUrl;

  // The 20 byte sha1 hash of the bencoded form of the info value from the
  // metainfo file.
  const std::array<char, 20> infoHash;

  // The total amount uploaded so far, encoded in base ten ascii.
  uint64_t uploaded;

  // The total amount downloaded so far, encoded in base ten ascii.
  uint64_t downloaded;

  // The number of bytes this peer still has to download, encoded in base ten
  // ascii.
  uint64_t left;

  Torrent(const std::string &trackerUrl,
                const std::array<char, 20> &infoHash, const uint64_t left,
                const uint64_t uploaded = 0,
                const uint64_t downloaded = 0);

  ~Torrent() = default;
};

#endif // TORRENT_H
