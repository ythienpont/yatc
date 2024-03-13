#include "TorrentParser.h"
#include <fstream>
#include <iostream>
#include <openssl/sha.h> // For SHA-1 hashing

TorrentParser *TorrentParser::pinstance{nullptr};
std::mutex TorrentParser::mutex;

TorrentParser *TorrentParser::getInstance() {
  std::lock_guard<std::mutex> lock(mutex);
  if (pinstance == nullptr) {
    pinstance = new TorrentParser();
  }
  return pinstance;
}

std::array<char, 20> compute_sha1(const std::string &data) {
  unsigned char hash[SHA_DIGEST_LENGTH];
  SHA1(reinterpret_cast<const unsigned char *>(data.c_str()), data.size(),
       hash);

  std::array<char, 20> result;
  std::copy(std::begin(hash), std::end(hash), result.begin());
  return result;
}

Torrent TorrentParser::parseTorrentFile(const std::string &filename) const {
  std::ifstream file(filename, std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open torrent file.");
  }

  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

  // Parse the content using your bencode library
  auto torrentDict = bencode::decode(content);

  // Assuming the successful decode gives a dictionary
  auto dict = std::get<bencode::dict>(torrentDict);

  // Extract the announce URL
  if (dict.find("announce") == dict.end()) {
    throw std::runtime_error("Torrent file does not contain announce URL.");
  }

  const std::string trackerUrl = std::get<std::string>(dict["announce"]);

  // Check and extract the info dictionary
  if (dict.find("info") == dict.end()) {
    throw std::runtime_error("Torrent file does not contain info dictionary.");
  }
  auto infoDict = std::get<bencode::dict>(dict["info"]);

  // Compute and store the info hash
  auto infoBencoded = bencode::encode(dict["info"]);
  std::array<char, 20> infoHash = compute_sha1(infoBencoded);

  // Initialize the Torrent object
  Torrent torrent;
  torrent.trackerUrl = trackerUrl;
  torrent.infoHash = infoHash;
  torrent.info = infoDict;

  // Extract piece length
  if (infoDict.find("piece length") != infoDict.end()) {
    torrent.pieceLength = std::get<bencode::integer>(infoDict["piece length"]);
  }

  // Extract pieces (the SHA1 hash for each piece)
  if (infoDict.find("pieces") != infoDict.end()) {
    auto pieces = std::get<std::string>(infoDict["pieces"]);
    for (size_t i = 0; i < pieces.size(); i += 20) {
      std::array<char, 20> pieceHash;
      std::copy_n(pieces.begin() + i, 20, pieceHash.begin());
      torrent.pieces.push_back(pieceHash);
    }
  }

  // Handle single and multiple file modes
  if (infoDict.find("files") != infoDict.end()) {
    // Multi-file torrent
    for (const auto &file : std::get<bencode::list>(infoDict["files"])) {
      auto fileDict = std::get<bencode::dict>(file);
      FileInfo fileInfo;
      fileInfo.length = std::get<bencode::integer>(fileDict["length"]);
      auto pathList = std::get<bencode::list>(fileDict["path"]);
      for (const auto &pathPart : pathList) {
        if (!fileInfo.path.empty())
          fileInfo.path += "/";
        fileInfo.path += std::get<std::string>(pathPart);
      }
      torrent.files.push_back(fileInfo);
      torrent.length += fileInfo.length;
    }
  } else if (infoDict.find("length") != infoDict.end()) {
    // Single-file torrent
    torrent.length = std::get<bencode::integer>(infoDict["length"]);
    if (infoDict.find("name") != infoDict.end()) {
      torrent.name = std::get<std::string>(infoDict["name"]);
      torrent.files.push_back({torrent.name, torrent.length});
    }
  }

  return torrent;
}
