#ifndef TRACKERCLIENT_H
#define TRACKERCLIENT_H

#include <array>
#include <cstdint>
#include <curl/curl.h>
#include <map>
#include <string>
#include <sstream>


template <size_t N>
inline std::string arrayToString(const std::array<char, N>& arr) {
    return std::string(arr.begin(), arr.end());
}

class TrackerClient {
public:
  using PeerId = std::array<char, 20>;

  struct Peer {
    std::string ip;
    uint16_t port;
  };

  struct TrackerResponse {
    std::string failureReason; // Empty if there's no failure
    uint16_t interval; // The number of seconds to wait between regular rerequests
    std::map<PeerId, Peer> peers;
  };

  enum class Event {
        Started, // When a download first begins
        Completed, // When the download is complete
        Stopped, // When the downloader ceases downloading
        Empty // Equivalent to not being present; for regular intervals
  };

  // Sends an announce request to the tracker with the current status of the torrent session.
  TrackerResponse announce(Event event = Event::Empty);

  TrackerClient(const std::string& trackerUrl, const std::array<char, 20>& infoHash,
      const std::array<char, 20>& peerId, uint16_t port, uint64_t uploaded,
      uint64_t downloaded, uint64_t left);

  ~TrackerClient() = default;

private:
  // The URL of the tracker server to which announce requests are sent.
  std::string trackerUrl;

  // The 20 byte sha1 hash of the bencoded form of the info value from the metainfo file.
  std::array<char, 20> infoHash;

  // A string of length 20 which this downloader uses as its id. Each downloader generates its own id at random at the start of a new download.
  PeerId peerId;

  // The port number this peer is listening on
  uint16_t port;

  // The total amount uploaded so far, encoded in base ten ascii.
  uint64_t uploaded;

  // The total amount downloaded so far, encoded in base ten ascii.
  uint64_t downloaded;

  // The number of bytes this peer still has to download, encoded in base ten ascii.
  uint64_t left;

  // Utility method to build the query string for announce requests.
  std::string buildQueryString(Event event);
};

#endif // TRACKERCLIENT_H
