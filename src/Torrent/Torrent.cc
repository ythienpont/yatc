#include "Torrent.h"


Torrent::Torrent(const std::string &trackerUrl,
                             const std::array<char, 20> &infoHash,
                             const uint64_t left,
                             const uint64_t uploaded, const uint64_t downloaded)
    : trackerUrl(trackerUrl), infoHash(infoHash),
      uploaded(uploaded), downloaded(downloaded), left(left) { }
