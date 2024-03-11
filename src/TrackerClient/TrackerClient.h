#ifndef TRACKERCLIENT_H
#define TRACKERCLIENT_H

#include <array>
#include <cstdint>
#include <curl/curl.h>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>

std::array<char, 20> generatePeerID();

class TrackerClient {
public:
  // Prefix used in creating a peer ID
  static const std::string PREFIX;

  using PeerId = std::array<char, 20>;

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
  };

  enum class Event {
    Started,   // When a download first begins
    Completed, // When the download is complete
    Stopped,   // When the downloader ceases downloading
    Empty      // Equivalent to not being present; for regular intervals
  };

  // Sends an announce request to the tracker with the current status of the
  // torrent session.
  TrackerResponse announce(Event event = Event::Empty);

  TrackerClient(const std::string &trackerUrl,
                const std::array<char, 20> &infoHash, const uint64_t left,
                const uint16_t port = 6881, const uint64_t uploaded = 0,
                const uint64_t downloaded = 0);

  ~TrackerClient() = default;

private:
  // The URL of the tracker server to which announce requests are sent.
  const std::string trackerUrl;

  // The 20 byte sha1 hash of the bencoded form of the info value from the
  // metainfo file.
  const std::array<char, 20> infoHash;

  // A string of length 20 which this downloader uses as its id. Each downloader
  // generates its own id at random at the start of a new download.
  PeerId peerId;

  // The port number this peer is listening on
  uint16_t port;

  // The total amount uploaded so far, encoded in base ten ascii.
  uint64_t uploaded;

  // The total amount downloaded so far, encoded in base ten ascii.
  uint64_t downloaded;

  // The number of bytes this peer still has to download, encoded in base ten
  // ascii.
  uint64_t left;

  // Utility method to build the query string for announce requests.
  std::string buildQueryString(Event event) const;
};

#endif // TRACKERCLIENT_H
