#include "Logger/Logger.h"
#include "TorrentClient/TorrentClient.h"

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
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <torrent file>\n";
    return 1;
  }

  Logger *log = Logger::getInstance();
  log->setOutput(&std::cerr);
  log->setLevel(Logger::DEBUG);

  TorrentClient client(argv[1]);

  client.start();
  client.stop();

  return 0;
}
