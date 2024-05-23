#ifndef TORRENTCLIENT_H
#define TORRENTCLIENT_H

#include "FileManager/FileManager.h"
#include "PeerConnection/PeerConnection.h"
#include "PieceManager/PieceManager.h"
#include "TorrentParser/TorrentParser.h"
#include "TrackerClient/TrackerClient.h"
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <vector>

/**
 * @class TorrentClient
 * @brief Manages torrent downloading and uploading, coordinating between
 * different components.
 *
 * This class integrates the FileManager, PieceManager, TorrentParser,
 * TrackerClient, and PeerConnections to manage the overall process of handling
 * a torrent, from parsing the file to managing downloads and uploads.
 */
class TorrentClient {
public:
  /**
   * @brief Constructs a TorrentClient with a specified torrent file.
   *
   * @param torrentFile The path to the torrent file to be managed.
   */
  TorrentClient(const std::string &torrentFile);

  /**
   * @brief Starts the torrent downloading process.
   *
   * Sets up torrent parsing, initializes file and piece management, connects to
   * peers, and starts the asynchronous I/O context.
   */
  void start();

  /**
   * @brief Stops all torrent downloading and uploading activities.
   *
   * Cleans up and gracefully shuts down all peer connections and tracker
   * sessions.
   */
  void stop();

private:
  boost::asio::io_context io_context_; ///< Boost ASIO I/O context for managing
                                       ///< asynchronous operations.
  std::unique_ptr<TrackerClient>
      trackerClient_; ///< Handles communication with the torrent tracker.
  std::shared_ptr<LinuxFileManager>
      fileManager_; ///< Manages file reading and writing for the torrent.
  std::unique_ptr<PieceManager>
      pieceManager_; ///< Manages the pieces of the torrent.
  std::unique_ptr<TorrentParser> torrentParser_; ///< Parses the torrent file.
  Torrent torrent_; ///< Torrent object holding parsed torrent data.
  std::vector<std::pair<Peer, std::unique_ptr<PeerConnection>>>
      peerConnections_; ///< Active connections to peers.

  /**
   * @brief Sets up the torrent from a file, parsing it and initializing the
   * file manager.
   *
   * @param torrentFile Path to the torrent file.
   */
  void setupTorrent(const std::string &torrentFile);

  /**
   * @brief Establishes connections to available peers.
   *
   * Uses information from the tracker to connect to peers for downloading and
   * uploading pieces.
   */
  void connectToPeers();

  /**
   * @brief Initiates a session with the tracker to get a list of peers.
   *
   * Contacts the tracker to update and receive information about other peers
   * sharing the same torrent.
   */
  void initiateTrackerSession();

  /**
   * @brief Manages the overall download process, coordinating piece requests
   * and downloads.
   */
  void handleDownload();

  /**
   * @brief Sends interest request to unchoke all connections
   */
  void sendInterest();

  /**
   * @brief Removes non-active peer connections.
   *
   * Periodically checks and removes peer connections that are no longer active
   * or responsive.
   */
  void pruneDeadConnections();
};

#endif // TORRENTCLIENT_H
