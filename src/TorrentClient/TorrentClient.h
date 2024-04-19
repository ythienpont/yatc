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

class TorrentClient {
public:
  TorrentClient(const std::string &torrentFile);
  void start();
  void stop();

private:
  boost::asio::io_context io_context_;
  std::unique_ptr<TrackerClient> trackerClient_;
  std::unique_ptr<FileManager> fileManager_;
  std::unique_ptr<PieceManager> pieceManager_;
  std::unique_ptr<TorrentParser> torrentParser_;
  Torrent torrent_;
  std::vector<std::pair<Peer, std::unique_ptr<PeerConnection>>>
      peerConnections_;

  void setupTorrent(const std::string &torrentFile);
  void connectToPeers();
  void initiateTrackerSession();
  void handleDownload();
  void pruneDeadConnections();
};

#endif // TORRENTCLIENT_H