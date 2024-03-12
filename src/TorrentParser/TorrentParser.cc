#include "TorrentParser.h"
#include <random>
#include <fstream>
#include<sstream>
#include <iostream>


TorrentParser* TorrentParser::pinstance{nullptr};
std::mutex TorrentParser::mutex;

TorrentParser *TorrentParser::getInstance()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (pinstance == nullptr)
    {
        pinstance = new TorrentParser();
    }
    return pinstance;
}

std::array<char, 20> generateRandom() {
  std::array<char, 20> peerId{};
  std::string PREFIX = "-YATC";

  std::copy(PREFIX.begin(), PREFIX.end(),
            peerId.begin());

  // Random device and generator
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 255);

  // Fill the rest of the array with random bytes
  for (size_t i = PREFIX.size(); i < peerId.size(); ++i) {
    peerId[i] = static_cast<char>(dis(gen));
  }

  return peerId;
}


Torrent TorrentParser::parseTorrentFile(const std::string& filename) const {

  std::ifstream file(filename, std::ios::binary);

  if (!file.is_open()) {
      throw std::runtime_error("Failed to open torrent file.");
    }
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  
    // Parse the content using your bencode library
    auto torrentDict = bencode::decode(content);

    // Assuming the successful decode gives a dictionary
    auto dict = std::get<bencode::dict>(torrentDict);
    std::array<char, 20> infoHash = generateRandom();

    // Compute and store the info hash
    if (dict.find("info") != dict.end()) {
      // Throw error
      printf("info found\n");
    }

    //std::string info = std::get<std::string>(dict["info"]);
    //std::cout <<info << std::endl;

    auto infoBencoded = bencode::encode(dict["info"]);

    // Extract the announce URL
    if (dict.find("announce") == dict.end()) {
      // Throw error
    }
    std::string trackerUrl = std::get<std::string>(dict["announce"]);
    std::cout <<trackerUrl << std::endl;


    if (dict.find("size") != dict.end()) {
      printf("size found\n");
      // Throw error
    }

    //std::string left = std::get<std::string>(dict["size"]);
    //std::cout <<left << std::endl;

  //uint64_t left = std::get<uint64_t>(dict["size"]);

    // Optional: Calculate file size (left) if necessary based on the info dictionary
    // This part is omitted for brevity but involves traversing the "info" dictionary
    // to sum the size of all files in case of multi-file torrents or just getting the
    // size for single-file torrents.

  //return Torrent(trackerUrl, infoHash, left);
  return Torrent("test.com/announce", infoHash, 0);
}
