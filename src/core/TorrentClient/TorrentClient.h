#ifndef TORRENTCLIENT_H
#define TORRENTCLIENT_H

#include "PeerConnection/PeerConnection.h"
#include "PieceManager/PieceManager.h"
#include "TorrentParser/TorrentParser.h"
#include "TrackerClient/TrackerClient.h"
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <vector>

class TorrentClient {
public:
  TorrentClient(const std::string &torrentFile);

  void start();

  void stop();

private:
  boost::asio::io_context io_context_;

  std::unique_ptr<TrackerClient> tracker_client_;
  std::shared_ptr<PieceManager> piece_manager_;
  std::unique_ptr<TorrentParser> torrent_parser_;
  Torrent torrent_;
  std::vector<std::shared_ptr<PeerConnection>> peer_connections_;

  std::mutex connections_mutex_;

  void setup_torrent(const std::string &torrent_file);

  void initiate_tracker_session();

  void add_connection(const Peer &peer);

  void handle_connect(std::shared_ptr<PeerConnection> connection,
                      const boost::system::error_code &error);
};

#endif // TORRENTCLIENT_H
