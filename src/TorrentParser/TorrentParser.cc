#include "TorrentParser.h"
#include "Torrent/Torrent.h"
#include "bencode.hpp"
#include <fstream>
#include <openssl/sha.h> // For SHA-1 hashing

TorrentParser *TorrentParser::pinstance_{nullptr};
std::mutex TorrentParser::mutex_;

TorrentParser *TorrentParser::getInstance() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (pinstance_ == nullptr) {
    pinstance_ = new TorrentParser();
  }
  return pinstance_;
}

std::array<std::byte, 20> compute_sha1(const std::string &data) {
  unsigned char hash[SHA_DIGEST_LENGTH]; // Ensure SHA_DIGEST_LENGTH is 20
  SHA1(reinterpret_cast<const unsigned char *>(data.c_str()), data.size(),
       hash);

  std::array<std::byte, 20> result;
  std::transform(std::begin(hash), std::end(hash), result.begin(),
                 [](unsigned char c) -> std::byte { return std::byte{c}; });
  return result;
}

std::string TorrentParser::readTorrentFile(const std::string &filename) const {
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open torrent file.");
  }
  return {std::istreambuf_iterator<char>(file),
          std::istreambuf_iterator<char>()};
}

bencode::dict TorrentParser::decodeContent(const std::string &content) const {
  return std::get<bencode::dict>(bencode::decode(content));
}

std::string TorrentParser::extractTrackerUrl(const bencode::dict &dict) const {
  if (dict.find("announce") == dict.end()) {
    throw std::runtime_error("Torrent file does not contain announce URL.");
  }
  return std::get<std::string>(dict.at("announce"));
}

bencode::dict TorrentParser::extractInfoDict(const bencode::dict &dict) const {
  if (dict.find("info") == dict.end()) {
    throw std::runtime_error("Torrent file does not contain info dictionary.");
  }
  return std::get<bencode::dict>(dict.at("info"));
}

InfoHash TorrentParser::computeInfoHash(const bencode::dict &infoDict) const {
  auto infoBencoded = bencode::encode(infoDict);
  return compute_sha1(
      infoBencoded); // Ensure this function is defined to compute SHA1 hash
}

void TorrentParser::extractFileInfo(Torrent &torrent,
                                    const bencode::dict &infoDict) const {
  if (infoDict.find("files") != infoDict.end()) { // Multi-file torrent
    const auto &files = std::get<bencode::list>(infoDict.at("files"));
    for (const auto &fileEntry : files) {
      const auto &fileDict = std::get<bencode::dict>(fileEntry);
      FileInfo fileInfo;
      fileInfo.length =
          (uint64_t)std::get<bencode::integer>(fileDict.at("length"));

      const auto &pathList = std::get<bencode::list>(fileDict.at("path"));
      for (const auto &pathPart : pathList) {
        if (!fileInfo.path.empty())
          fileInfo.path += "/";
        fileInfo.path += std::get<std::string>(pathPart);
      }

      torrent.files.push_back(std::move(fileInfo));
    }
  } else { // Single-file torrent
    torrent.files.emplace_back(FileInfo{
        std::get<std::string>(infoDict.at("name")), // File name
        (uint64_t)std::get<bencode::integer>(
            infoDict.at("length")) // File length
    });
  }
}

void TorrentParser::extractPieces(Torrent &torrent,
                                  const bencode::dict &infoDict) const {
  torrent.pieceLength = std::get<bencode::integer>(
      infoDict.at("piece length")); // Assuming piece length is stored as int

  const auto &piecesString = std::get<std::string>(infoDict.at("pieces"));
  for (size_t i = 0; i < piecesString.size(); i += 20) {
    InfoHash pieceHash;
    // Use std::transform to explicitly cast each char to std::byte
    std::transform(piecesString.begin() + i, piecesString.begin() + i + 20,
                   pieceHash.begin(), [](char c) {
                     return std::byte{static_cast<unsigned char>(c)};
                   });
    torrent.pieces.push_back(pieceHash);
  }
}

Torrent TorrentParser::parseTorrentFile(const std::string &filename) const {
  std::string content = readTorrentFile(filename);
  auto dict = decodeContent(content);

  Torrent torrent;
  torrent.trackerUrl = extractTrackerUrl(dict);
  auto infoDict = extractInfoDict(dict);
  torrent.infoHash = computeInfoHash(infoDict);
  torrent.info = infoDict;

  extractFileInfo(torrent, infoDict);
  extractPieces(torrent, infoDict);

  return torrent;
}
