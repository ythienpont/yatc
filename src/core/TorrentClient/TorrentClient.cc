#include "TorrentClient.h"
#include "Logger/Logger.h"
#include <iostream>
#include <memory>

TorrentClient::TorrentClient(const std::string &torrent_file) {
  setup_torrent(torrent_file);
}

void TorrentClient::start() {
  Logger::instance()->log("Initiating tracker session...");
  initiate_tracker_session();
  io_context_.run();
}

void TorrentClient::stop() { io_context_.stop(); }

void TorrentClient::setup_torrent(const std::string &torrent_file) {
  Logger *logger = Logger::instance();
  logger->log("Initializing torrent setup...");

  try {
    torrent_parser_ = std::make_unique<TorrentParser>();
    logger->log("Torrent parser initialized.");
  } catch (const std::exception &e) {
    logger->log("Error initializing torrent parser: " + std::string(e.what()));
    return;
  }

  try {
    torrent_ = torrent_parser_->parse_torrent_file(torrent_file);
    logger->log("Torrent file parsed: " + torrent_file);
  } catch (const std::exception &e) {
    logger->log("Error parsing torrent file: " + std::string(e.what()));
    return;
  }

  try {
    piece_manager_ =
        std::make_shared<PieceManager>(torrent_.size(), torrent_.piece_length);
    logger->log("Piece manager set up for " +
                std::to_string(torrent_.total_pieces()) + " pieces.");
  } catch (const std::exception &e) {
    logger->log("Error setting up piece manager: " + std::string(e.what()));
    return;
  }

  try {
    file_manager_ = std::make_shared<LinuxFileManager>(
        torrent_.files, torrent_.piece_length, torrent_.pieces);
  } catch (const std::exception &e) {
    logger->log("Error setting up file manager: " + std::string(e.what()));
    return;
  }

  logger->log("Torrent setup complete.");
}

void TorrentClient::initiate_tracker_session() {
  int retry_count = 0;
  const int max_retries = 3; // Maximum number of retry attempts
  while (retry_count < max_retries) {
    tracker_client_ = std::make_unique<TrackerClient>(torrent_);
    try {
      TrackerResponse response =
          tracker_client_->announce(TrackerClient::Event::Started);

      for (auto peer : response.peers) {
        add_connection(peer);
      }

      // Successfully connected and received response
      break;
    } catch (const std::exception &e) {
      retry_count++;

      std::cerr << "Tracker connection failed: " << e.what() << ". Retrying ("
                << retry_count << "/" << max_retries << ")" << std::endl;
      if (retry_count == max_retries) {
        throw; // Rethrow the exception
      }
    }
  }
}

void TorrentClient::add_connection(const Peer &peer) {
  auto connection = std::make_shared<PeerConnection>(
      io_context_, torrent_.info_hash, tracker_client_->getPeerId(),
      piece_manager_, file_manager_);
  {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    peer_connections_.push_back(connection);
  }
  tcp::resolver resolver(io_context_);
  auto endpoints = resolver.resolve(peer.ip, std::to_string(peer.port));
  boost::asio::async_connect(connection->socket(), endpoints,
                             boost::bind(&TorrentClient::handle_connect, this,
                                         connection,
                                         boost::asio::placeholders::error));
}

void TorrentClient::handle_connect(std::shared_ptr<PeerConnection> connection,
                                   const boost::system::error_code &error) {
  if (!error) {
    connection->start();
  } else {
    std::cerr << "Connect error: " << error.message() << std::endl;
    {
      std::lock_guard<std::mutex> lock(connections_mutex_);
      auto it = std::remove(peer_connections_.begin(), peer_connections_.end(),
                            connection);
      peer_connections_.erase(it, peer_connections_.end());
    }
  }
}
