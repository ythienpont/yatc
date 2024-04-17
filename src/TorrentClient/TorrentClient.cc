#include "TorrentClient.h"

TorrentClient::TorrentClient(const std::string &torrentFile) {
  setupTorrent(torrentFile);
}

void TorrentClient::start() {
  initiateTrackerSession();
  connectToPeers();
  handleDownload();
}

void TorrentClient::stop() { io_context_.stop(); }

void TorrentClient::setupTorrent(const std::string &torrentFile) {
  torrentParser_ = std::make_unique<TorrentParser>();
  torrent_ = torrentParser_->parseTorrentFile(torrentFile);
  fileManager_ = std::make_unique<FileManager>(torrent_);
  pieceManager_ = std::make_unique<PieceManager>(torrent_.totalPieces());
}

void TorrentClient::initiateTrackerSession() {
  int retryCount = 0;
  const int maxRetries = 3; // Maximum number of retry attempts
  while (retryCount < maxRetries) {
    try {
      trackerClient_ = std::make_unique<TrackerClient>(torrent_);

      TrackerResponse response =
          trackerClient_->announce(TrackerClient::Event::Started);

      peers_ = response.peers;
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
  // TODO: Update peers with interval
}

void TorrentClient::connectToPeers() {
  for (const auto &peer : peers_) {
    auto peerConn = std::make_shared<PeerConnection>(
        io_context_, peer, trackerClient_->getPeerId(), torrent_.infoHash);
    peerConnections_.push_back(peerConn);
    peerConn->handshake();
  }
}

void TorrentClient::handleDownload() {
  // Start the asio context in a separate thread or run it directly if the
  // client is single-threaded.
  io_context_.run();
  // TODO: Post-download handling (e.g., verifying download, re-seeding)
}
