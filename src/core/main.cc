#include "Logger/Logger.h"
#include "TorrentClient/TorrentClient.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <torrent file>\n";
    return 1;
  }

  Logger *log = Logger::instance();
  log->setOutput(&std::cerr);
  log->setLevel(Logger::DEBUG);

  TorrentClient client(argv[1]);

  client.start();
  client.stop();

  return 0;
}
