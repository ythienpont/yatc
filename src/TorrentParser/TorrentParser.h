#ifndef TORRENTPARSER_H
#define TORRENTPARSER_H

#include "../Torrent/Torrent.h"
#include <bencode.hpp>
#include <openssl/sha.h>
#include <string>

class TorrentParser {
private:
  // Reads the entire content of a torrent file into a string.
  std::string readTorrentFile(const std::string &filename) const;

  // Decodes a bencoded string into a dictionary.
  bencode::dict decodeContent(const std::string &content) const;

  // Extracts the tracker URL from the torrent's metainfo dictionary.
  std::string extractTrackerUrl(const bencode::dict &dict) const;

  // Extracts the 'info' dictionary from the torrent's metainfo dictionary,
  // which contains details about the files and pieces.
  bencode::dict extractInfoDict(const bencode::dict &dict) const;

  // Computes the SHA-1 hash of the bencoded 'info' dictionary. This hash is
  // used as the torrent's InfoHash to verify the integrity and identity of the
  // torrent.
  InfoHash computeInfoHash(const bencode::dict &infoDict) const;

  // Extracts file information from the 'info' dictionary and populates the
  // given Torrent object with it. This includes setting file paths, lengths,
  // and other relevant data.
  void extractFileInfo(Torrent &torrent, const bencode::dict &infoDict) const;

  // Extracts the piece hashes from the 'info' dictionary and stores them in the
  // Torrent object.
  void extractPieces(Torrent &torrent, const bencode::dict &infoDict) const;

public:
  TorrentParser() = default;
  ~TorrentParser() = default;

  // Parses a torrent file and returns a Torrent object containing all the
  // relevant data.
  Torrent parseTorrentFile(const std::string &filename) const;
};

#endif // TORRENTPARSER_H
