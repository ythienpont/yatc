#include "TrackerClient/TrackerClient.h"
#include "TorrentParser/TorrentParser.h"
#include "Torrent/Torrent.h"
#include <iostream>

std::array<char, 20> hexStringToByteArray(const std::string &hex) {
  if (hex.length() != 40) {
    throw std::invalid_argument("Invalid input length for an info hash");
  }

  std::array<char, 20> bytes;
  for (size_t i = 0, j = 0; i < hex.length(); i += 2, ++j) {
    std::string byteString = hex.substr(i, 2);
    char byte = static_cast<char>(std::stoi(byteString, nullptr, 16));
    bytes[j] = byte;
  }
  return bytes;
}

int main(int argc, char *argv[]) {
  Torrent torrent = TorrentParser::getInstance()->parseTorrentFile(argv[1]);
  TrackerClient tc(torrent);

  //TrackerResponse response = tc.announce(TrackerClient::Event::Started);
  //std::cout << response.toString() << std::endl;
  return 0;
}
