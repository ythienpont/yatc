#ifndef TRACKERCLIENT_H
#define TRACKERCLIENT_H

#include "../Torrent/Torrent.h"
#include <array>
#include <cstdint>
#include <curl/curl.h>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>

std::array<std::byte, 20> generatePeerId();

using PeerId = std::array<std::byte, 20>;

struct Peer {
  std::string ip;
  uint16_t port; // Port at which the torrent service is running
};

struct TrackerResponse {
  // Empty if there's no failure
  std::string failureReason;

  // The number of seconds to wait between regular rerequests
  uint16_t interval;

  // List of all available peers
  std::map<PeerId, Peer> peers;

  std::string toString() const;
};

class TrackerClient {
public:
  // Prefix used in creating a peer ID
  static const std::string PREFIX;

  enum class Event {
    Started,   // When a download first begins
    Completed, // When the download is complete
    Stopped,   // When the downloader ceases downloading
    Empty      // Equivalent to not being present; for regular intervals
  };

  // Sends an announce request to the tracker with the current status of the
  // torrent session.
  TrackerResponse announce(Event event = Event::Empty);

  TrackerClient(const Torrent &torrent, const uint16_t port = 6881);

  ~TrackerClient() = default;

private:
  Torrent torrent_;

  // A string of length 20 which this downloader uses as its id. Each downloader
  // generates its own id at random at the start of a new download.
  PeerId peerId_;

  // The port number this peer is listening on
  uint16_t port_;

  // The total amount uploaded so far
  uint64_t uploaded_;

  // The total amount downloaded so far
  uint64_t downloaded_;

  // The number of bytes this peer still has to download
  uint64_t left_;

  // Utility method to build the query string for announce requests.
  std::string buildQueryString(Event event) const;
};

#endif // TRACKERCLIENT_H
