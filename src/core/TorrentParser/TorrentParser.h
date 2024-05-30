#ifndef TORRENTPARSER_H
#define TORRENTPARSER_H

#include "Torrent/Torrent.h"
#include <bencode.hpp>
#include <openssl/sha.h>
#include <string>

/**
 * @brief Provides functionality to parse torrent files.
 *
 * The TorrentParser class reads torrent files, decodes their contents, and
 * extracts key information such as tracker URLs, file details, and piece hashes
 * necessary for torrent downloading.
 */
class TorrentParser {
private:
  /**
   * @brief Reads the entire content of a torrent file into a string.
   *
   * @param filename Path to the torrent file.
   * @return std::string The content of the torrent file as a string.
   */
  std::string read_torrent_file(const std::string &filename) const;

  /**
   * @brief Decodes a bencoded string into a dictionary.
   *
   * @param content The bencoded string to decode.
   * @return bencode::dict A dictionary representing the decoded content.
   */
  bencode::dict decode_content(const std::string &content) const;

  /**
   * @brief Extracts the tracker URL from the torrent's metainfo dictionary.
   *
   * @param dict The metainfo dictionary.
   * @return std::string The extracted tracker URL.
   */
  std::string extract_tracker_url(const bencode::dict &dict) const;

  /**
   * @brief Extracts the 'info' dictionary from the torrent's metainfo
   * dictionary.
   *
   * This dictionary contains details about the files and pieces in the torrent.
   * @param dict The metainfo dictionary.
   * @return bencode::dict The 'info' dictionary.
   */
  bencode::dict extract_info_dict(const bencode::dict &dict) const;

  /**
   * @brief Computes the SHA-1 hash of the bencoded 'info' dictionary.
   *
   * This hash is used as the torrent's InfoHash to verify the integrity and
   * identity of the torrent.
   * @param info_dict The 'info' dictionary.
   * @return InfoHash The computed SHA-1 hash.
   */
  InfoHash compute_info_hash(const bencode::dict &info_dict) const;

  /**
   * @brief Extracts file information from the 'info' dictionary and populates
   * the given Torrent object with it.
   *
   * This includes setting file paths, lengths, and other relevant data.
   * @param torrent Reference to the Torrent object to populate.
   * @param info_dict The 'info' dictionary containing file details.
   */
  void extract_file_info(Torrent &torrent,
                         const bencode::dict &info_dict) const;

  /**
   * @brief Extracts the piece hashes from the 'info' dictionary and stores them
   * in the Torrent object.
   *
   * @param torrent Reference to the Torrent object to populate.
   * @param info_dict The 'info' dictionary containing piece hashes.
   */
  void extract_pieces(Torrent &torrent, const bencode::dict &info_dict) const;

public:
  /**
   * @brief Default constructor for TorrentParser.
   */
  TorrentParser() = default;

  /**
   * @brief Default destructor for TorrentParser.
   */
  ~TorrentParser() = default;

  /**
   * @brief Parses a torrent file and returns a Torrent object containing all
   * relevant data.
   *
   * @param filename Path to the torrent file to parse.
   * @return Torrent A Torrent object populated with data from the torrent file.
   */
  Torrent parse_torrent_file(const std::string &filename) const;
};

#endif // TORRENTPARSER_H
