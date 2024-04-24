#include "TorrentClient.h"
#include <algorithm>

TorrentClient::TorrentClient(const std::string &torrentFile) {
  setupTorrent(torrentFile);
}

void TorrentClient::pruneDeadConnections() {
  peerConnections_.erase(
      std::remove_if(
          peerConnections_.begin(), peerConnections_.end(),
          [](const std::pair<Peer, std::unique_ptr<PeerConnection>> &pair) {
            return !pair.second->isConnected();
          }),
      peerConnections_.end());
}

void TorrentClient::start() {
  initiateTrackerSession();
  connectToPeers();
  io_context_.run();

  pruneDeadConnections();
  handleDownload();
}

void TorrentClient::stop() { io_context_.stop(); }

void TorrentClient::setupTorrent(const std::string &torrentFile) {
  torrentParser_ = std::make_unique<TorrentParser>();
  torrent_ = torrentParser_->parseTorrentFile(torrentFile);
  fileManager_ = std::make_unique<LinuxFileManager>(
      torrent_.files,
      torrent_.pieceLength); // TODO: Currently only support linux
  pieceManager_ = std::make_unique<PieceManager>(torrent_.totalPieces());
}

void TorrentClient::initiateTrackerSession() {
  int retryCount = 0;
  const int maxRetries = 3; // Maximum number of retry attempts
  while (retryCount < maxRetries) {
    trackerClient_ = std::make_unique<TrackerClient>(torrent_);
    try {
      TrackerResponse response =
          trackerClient_->announce(TrackerClient::Event::Started);

      for (auto peer : response.peers) {
        peerConnections_.emplace_back(peer, nullptr);
      }

      // Successfully connected and received response
      break;
    } catch (const std::exception &e) {
      retryCount++;

      std::cerr << "Tracker connection failed: " << e.what() << ". Retrying ("
                << retryCount << "/" << maxRetries << ")" << std::endl;
      if (retryCount == maxRetries) {
        throw; // Rethrow the exception
      }
    }
  }
}

void TorrentClient::connectToPeers() {
  for (auto &[peer, connection] : peerConnections_) {
    if (connection == nullptr) { // Check if the unique_ptr is empty
      connection = std::make_unique<PeerConnection>(
          io_context_, peer, trackerClient_->getPeerId(), torrent_.infoHash);
    }
    connection->handshake();
  }
}

void TorrentClient::handleDownload() {
  // Start the asio context in a separate thread or run it directly if the
  // client is single-threaded.
  //
  // TODO: Post-download handling (e.g., verifying download, re-seeding)
}
