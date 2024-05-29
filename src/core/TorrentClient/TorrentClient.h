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
 * @brief Manages the overall process of downloading and uploading a torrent.
 */
class TorrentClient {
public:
  /**
   * @brief Constructs a TorrentClient with the specified torrent file.
   *
   * @param torrentFile The path to the .torrent file.
   */
  TorrentClient(const std::string &torrentFile);

  /**
   * @brief Starts the torrent client.
   *
   * Initiates the downloading and uploading process.
   */
  void start();

  /**
   * @brief Stops the torrent client.
   *
   * Gracefully shuts down all connections and stops the torrent process.
   */
  void stop();

  std::string download_info() { return "YAY!"; };

private:
  boost::asio::io_context
      io_context_; ///< IO context for managing asynchronous operations.

  std::unique_ptr<TrackerClient>
      tracker_client_; ///< Manages communication with the tracker.
  std::shared_ptr<PieceManager>
      piece_manager_; ///< Manages the pieces of the torrent.
  std::shared_ptr<LinuxFileManager>
      file_manager_; ///< Manages file operations on Linux.
  std::unique_ptr<TorrentParser> torrent_parser_; ///< Parses the .torrent file.
  Torrent torrent_;                               ///< The torrent metadata.
  std::vector<std::shared_ptr<PeerConnection>>
      peer_connections_; ///< List of peer connections.

  std::mutex
      connections_mutex_; ///< Mutex for thread-safe access to peer connections.

  /**
   * @brief Sets up the torrent by parsing the .torrent file.
   *
   * @param torrent_file The path to the .torrent file.
   */
  void setup_torrent(const std::string &torrent_file);

  /**
   * @brief Initiates the session with the tracker.
   */
  void initiate_tracker_session();

  /**
   * @brief Adds a connection to a peer.
   *
   * @param peer The peer to connect to.
   */
  void add_connection(const Peer &peer);

  /**
   * @brief Handles the result of attempting to connect to a peer.
   *
   * @param connection The peer connection.
   * @param error The error code resulting from the connection attempt.
   */
  void handle_connect(std::shared_ptr<PeerConnection> connection,
                      const boost::system::error_code &error);
};

#endif // TORRENTCLIENT_H
