#include "TorrentClient.h"

TorrentClient::TorrentClient(const std::string &torrentFile) {
  setupTorrent(torrentFile);
}

void TorrentClient::start() {
  initiateTrackerSession();
  connectToPeers();
  handleDownload();
}

void TorrentClient::stop() { io_context.stop(); }

void TorrentClient::setupTorrent(const std::string &torrentFile) {
  torrentParser = std::make_unique<TorrentParser>();
  torrent = torrentParser->parseTorrentFile(torrentFile);
  // fileManager = std::make_unique<FileManager>();
  //  pieceManager = std::make_unique<PieceManager>(filepaths, piece_size);
}

void TorrentClient::initiateTrackerSession() {
  trackerClient = std::make_unique<TrackerClient>(torrent);
  TrackerResponse response =
      trackerClient->announce(TrackerClient::Event::Started);
  // TODO: Check for failure reason
  peers_ = response.peers;
  // TODO: Update peers with interval
}

void TorrentClient::connectToPeers() {
  for (const auto &peer : peers_) {
    auto peerConn = std::make_shared<PeerConnection>(
        io_context, peer, trackerClient->getPeerId(), torrent.infoHash);
    peerConnections.push_back(peerConn);
    peerConn->handshake();
    // TODO: Actually download
  }
}

void TorrentClient::handleDownload() {
  // Start the asio context in a separate thread or run it directly if the
  // client is single-threaded.
  io_context.run();
  // TODO: Post-download handling (e.g., verifying download, re-seeding)
}
