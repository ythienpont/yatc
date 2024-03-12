#ifndef TORRENTPARSER_H
#define TORRENTPARSER_H

#include <iterator>
#include <mutex>
#include <openssl/sha.h> // For SHA-1 hashing
#include <string>
#include <stdexcept>
#include "../Torrent/Torrent.h"
#include "../../lib/bencode.hpp"

class TorrentParser {
private:
  static TorrentParser* pinstance;
  static std::mutex mutex;
protected:
  TorrentParser() = default;
  ~TorrentParser() = default;
public:
  TorrentParser(TorrentParser &other) = delete;
  void operator=(const TorrentParser&) = delete;

  static TorrentParser* getInstance();

  Torrent parseTorrentFile(const std::string& filename) const;
};

#endif // TORRENTPARSER_H
