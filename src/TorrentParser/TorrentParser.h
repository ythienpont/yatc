#ifndef TORRENTPARSER_H
#define TORRENTPARSER_H

#include "../Torrent/Torrent.h"
#include <bencode.hpp>
#include <iterator>
#include <openssl/sha.h> // For SHA-1 hashing
#include <stdexcept>
#include <string>

class TorrentParser {
private:
  std::string readTorrentFile(const std::string &filename) const;
  bencode::dict decodeContent(const std::string &content) const;
  std::string extractTrackerUrl(const bencode::dict &dict) const;
  bencode::dict extractInfoDict(const bencode::dict &dict) const;
  InfoHash computeInfoHash(const bencode::dict &infoDict) const;
  void extractFileInfo(Torrent &torrent, const bencode::dict &infoDict) const;
  void extractPieces(Torrent &torrent, const bencode::dict &infoDict) const;

public:
  TorrentParser() = default;
  ~TorrentParser() = default;

  Torrent parseTorrentFile(const std::string &filename) const;
};

#endif // TORRENTPARSER_H
