#ifndef TRACKERCLIENT_H
#define TRACKERCLIENT_H

#include "Logger/Logger.h"
#include "PeerConnection/PeerConnection.h"
#include "Torrent/Torrent.h"
#include <cstdint>
#include <curl/curl.h>
#include <string>

/**
 * @brief Generates a unique peer ID for use in peer-to-peer connections.
 *
 * @return Peer::Id Generated unique peer ID.
 */
Peer::Id generatePeerId();

/**
 * @brief Represents a response from the tracker.
 *
 * This structure holds the details of the tracker's response including any
 * failure reasons, the advised wait interval between requests, and the list
 * of peers.
 */
struct TrackerResponse {
  /// @brief Description of failure if the request failed; empty otherwise.
  std::string failureReason;

  /// @brief The number of seconds the client should wait before making a
  /// rerequest.
  uint16_t interval;

  /// @brief List of peers returned by the tracker.
  std::vector<Peer> peers;
};

/**
 * @brief Handles communication with the tracker for a torrent session.
 *
 * This class provides functionality to send announce requests to the tracker
 * and handle the responses. It manages the state of the torrent session,
 * including tracking downloaded and uploaded amounts.
 */
class TrackerClient {
public:
  /// @brief Peer ID prefix used when creating a new peer ID.
  static const std::string PREFIX;

  /**
   * @brief Enumeration of possible events to send to the tracker.
   */
  enum class Event {
    Started,   ///< Event when a download first begins.
    Completed, ///< Event when the download is complete.
    Stopped,   ///< Event when the downloader ceases downloading.
    Empty      ///< Used for regular interval updates without specific events.
  };

  /**
   * @brief Sends an announce request to the tracker to update the current
   * status of the torrent session.
   *
   * @param event The type of event to report to the tracker; defaults to
   * Event::Empty.
   * @return TrackerResponse The response from the tracker.
   */

  TrackerResponse announce(Event event = Event::Empty);

  /**
   * @brief Retrieves the peer ID used by this client.
   *
   * @return Peer::Id The peer ID.
   */
  Peer::Id peer_id() const { return peerId_; }

  /**
   * @brief Retrieves the info hash of the torrent.
   *
   * @return InfoHash The info hash of the torrent.
   */
  InfoHash info_hash() const { return torrent_.info_hash; }

  /**
   * @brief Constructs a new Tracker Client object.
   *
   * @param torrent The torrent associated with this client.
   * @param port The port number this peer listens on; defaults to 6881.
   */
  TrackerClient(const Torrent &torrent, const uint16_t port = 6881);

  /// @brief Default destructor.
  ~TrackerClient() = default;

private:
  /// @brief The torrent associated with this tracker client.
  Torrent torrent_;

  /// @brief This downloader's peer ID, randomly generated at the start of a new
  /// download.
  Peer::Id peerId_;

  /// @brief Port number on which this peer listens.
  uint16_t port_;

  /// @brief Total amount uploaded so far.
  uint64_t uploaded_;

  /// @brief Total amount downloaded so far.
  uint64_t downloaded_;

  /// @brief Number of bytes this peer still needs to download.
  uint64_t left_;

  Logger *logger_;

  /**
   * @brief Builds the query string for announce requests based on the event
   * type.
   *
   * @param event The event type to report in the query string.
   * @return std::string The constructed query string.
   */
  std::string buildQueryString(Event event) const;
};

#endif // TRACKERCLIENT_H
