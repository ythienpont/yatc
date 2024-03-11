#ifndef TORRENTPARSER_H
#define TORRENTPARSER_H

#include <mutex>

class TorrentParser {
private:
  static TorrentParser* pinstance;
  static std::mutex mutex;
protected:
  TorrentParser() = default;
  ~TorrentParser() = default;
public:
  TorrentParser(TorrentParser &other) = delete;
  void operator=(const TorrentParser&) = delete;

  static TorrentParser* getInstance();

  void parse() const;
};

TorrentParser* TorrentParser::pinstance{nullptr};
std::mutex TorrentParser::mutex;

#endif // TORRENTPARSER_H
