#include "TrackerClient/TrackerClient.h"

std::array<char, 20> hexStringToByteArray(const std::string& hex) {
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
  TrackerClient tc("https://ipv6.torrent.ubuntu.com/announce",
      hexStringToByteArray("9ecd4676fd0f0474151a4b74a5958f42639cebdf"),
      5173995520);

  tc.announce(TrackerClient::Event::Started);

  return 0;
}
