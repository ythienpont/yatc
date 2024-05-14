#include "TorrentClient.h"
#include "Logger/Logger.h"
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
  Logger::getInstance()->log("Initiating tracker session...");
  initiateTrackerSession();
  connectToPeers();
  io_context_.run();
}

void TorrentClient::stop() { io_context_.stop(); }

void TorrentClient::setupTorrent(const std::string &torrentFile) {
  Logger *logger = Logger::getInstance();
  logger->log("Initializing torrent setup...");

  try {
    torrentParser_ = std::make_unique<TorrentParser>();
    logger->log("Torrent parser initialized.");
  } catch (const std::exception &e) {
    logger->log("Error initializing torrent parser: " + std::string(e.what()));
    return;
  }

  try {
    torrent_ = torrentParser_->parseTorrentFile(torrentFile);
    logger->log("Torrent file parsed: " + torrentFile);
  } catch (const std::exception &e) {
    logger->log("Error parsing torrent file: " + std::string(e.what()));
    return;
  }

  try {
    fileManager_ = std::make_unique<LinuxFileManager>(torrent_.files,
                                                      torrent_.pieceLength);
    logger->log("File manager created for " +
                std::to_string(torrent_.files.size()) + " file(s).");
  } catch (const std::exception &e) {
    logger->log("Error creating file manager: " + std::string(e.what()));
    return;
  }

  try {
    pieceManager_ = std::make_unique<PieceManager>(torrent_.totalPieces());
    logger->log("Piece manager set up for " +
                std::to_string(torrent_.totalPieces()) + " pieces.");
  } catch (const std::exception &e) {
    logger->log("Error setting up piece manager: " + std::string(e.what()));
    return;
  }

  logger->log("Torrent setup complete.");
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
