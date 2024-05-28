#include "TorrentParser.h"
#include "Logger/Logger.h"
#include <cstdint>
#include <fstream>
#include <stdexcept>

std::array<std::byte, 20> compute_sha1(const std::string &data) {
  unsigned char hash[SHA_DIGEST_LENGTH]; // Ensure SHA_DIGEST_LENGTH is 20
  SHA1(reinterpret_cast<const unsigned char *>(data.c_str()), data.size(),
       hash);

  std::array<std::byte, 20> result;
  std::transform(std::begin(hash), std::end(hash), result.begin(),
                 [](unsigned char c) -> std::byte { return std::byte{c}; });
  return result;
}

std::string
TorrentParser::read_torrent_file(const std::string &filename) const {
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open torrent file.");
  }
  return {std::istreambuf_iterator<char>(file),
          std::istreambuf_iterator<char>()};
}

bencode::dict TorrentParser::decode_content(const std::string &content) const {
  return std::get<bencode::dict>(bencode::decode(content));
}

std::string
TorrentParser::extract_tracker_url(const bencode::dict &dict) const {
  if (dict.find("announce") == dict.end()) {
    throw std::runtime_error("Torrent file does not contain announce URL.");
  }
  return std::get<std::string>(dict.at("announce"));
}

bencode::dict
TorrentParser::extract_info_dict(const bencode::dict &dict) const {
  if (dict.find("info") == dict.end()) {
    throw std::runtime_error("Torrent file does not contain info dictionary.");
  }
  return std::get<bencode::dict>(dict.at("info"));
}

InfoHash
TorrentParser::compute_info_hash(const bencode::dict &info_dict) const {
  auto info_bencoded = bencode::encode(info_dict);
  return compute_sha1(info_bencoded);
}

void TorrentParser::extract_file_info(Torrent &torrent,
                                      const bencode::dict &info_dict) const {
  if (info_dict.find("files") != info_dict.end()) { // Multi-file torrent
    const auto &files = std::get<bencode::list>(info_dict.at("files"));

    // Starting offset for file start
    uint64_t current_offset = 0;

    for (const auto &file_entry : files) {
      const auto &file_dict = std::get<bencode::dict>(file_entry);
      FileInfo file_info;
      file_info.length =
          (uint64_t)std::get<bencode::integer>(file_dict.at("length"));

      // Construct the path from the path list
      const auto &path_list = std::get<bencode::list>(file_dict.at("path"));
      for (const auto &path_part : path_list) {
        if (!file_info.path.empty()) {
          file_info.path += "/";
        }
        file_info.path += std::get<std::string>(path_part);
      }

      // Calculate start and end offsets based on the current cumulative offset
      file_info.start_offset = current_offset;
      file_info.end_offset = current_offset + file_info.length - 1;

      // Update the current offset for the next file
      current_offset += file_info.length;

      torrent.files.push_back(std::move(file_info));
    }

  } else { // Single-file torrent
    torrent.files.emplace_back(FileInfo{
        std::get<std::string>(info_dict.at("name")), // File name
        (uint64_t)std::get<bencode::integer>(
            info_dict.at("length")) // File length
    });
  }
}

void TorrentParser::extract_pieces(Torrent &torrent,
                                   const bencode::dict &info_dict) const {
  torrent.piece_length = static_cast<uint32_t>(std::get<bencode::integer>(
      info_dict.at("piece length"))); // Assuming piece length is stored as int

  const auto &pieces_string = std::get<std::string>(info_dict.at("pieces"));
  for (size_t i = 0; i < pieces_string.size(); i += 20) {
    InfoHash piece_hash;

    std::transform(pieces_string.begin() + i, pieces_string.begin() + i + 20,
                   piece_hash.begin(), [](char c) {
                     return std::byte{static_cast<unsigned char>(c)};
                   });

    torrent.pieces.push_back(piece_hash);
  }
}

Torrent TorrentParser::parse_torrent_file(const std::string &filename) const {
  std::string content = read_torrent_file(filename);
  auto dict = decode_content(content);

  Torrent torrent;
  torrent.tracker_url = extract_tracker_url(dict);
  auto info_dict = extract_info_dict(dict);
  torrent.info_hash = compute_info_hash(info_dict);
  torrent.info = info_dict;

  extract_file_info(torrent, info_dict);
  extract_pieces(torrent, info_dict);

  Logger::instance()->log(torrent.diagnostic_info(), Logger::DEBUG);

  return torrent;
}
