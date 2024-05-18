#ifndef PEERCONNECTION_H
#define PEERCONNECTION_H

#include "Logger/Logger.h"
#include "Message/Message.h"
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/bind/bind.hpp>
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

using namespace boost::placeholders;
using tcp = boost::asio::ip::tcp;

struct Peer {
  using Id = std::array<std::byte, 20>;
  std::optional<Id> id;
  std::string ip;
  uint16_t port; // Port at which the torrent service is running
  bool isIdSet() const { return id.has_value(); }
};

struct ConnectionState {
  // Local state
  bool localInterested{false};
  bool localChoked{true};

  // Remote state
  bool remoteInterested{false};
  bool remoteChoked{true};

  ConnectionState() = default;
};

class PeerConnection {
public:
  PeerConnection(boost::asio::io_context &ioContext, const Peer peer,
                 const std::array<std::byte, 20> myPeerId,
                 const std::array<std::byte, 20> infoHash,
                 const size_t totalPieces)
      : ioContext_(ioContext), socket_(ioContext), peer_(peer),
        myPeerId_(myPeerId), infoHash_(infoHash), pieces_(totalPieces, false),
        readBuffer_(1024), state_() {
    logger = Logger::instance();
  }

  // Establish connection and perform handshake
  void handshake();

  // Request a piece of the file
  void sendRequest(int pieceIndex);

  // Handle receiving a piece of the file
  void receivePiece(int pieceIndex, const std::vector<uint8_t> &pieceData);

  // Close the connection
  void closeConnection();

  // Check if the connection is active
  bool isConnected() const;

private:
  boost::asio::io_context &ioContext_;
  tcp::socket socket_;
  Peer peer_;
  std::array<std::byte, 20> myPeerId_;
  std::array<std::byte, 20> infoHash_;
  std::vector<bool> pieces_; // Tracking which pieces this peer has
  std::vector<std::byte> writeBuffer_;
  std::vector<std::byte> readBuffer_;
  std::vector<std::byte> messageBuffer_;
  std::atomic<bool> isActive_{false};
  ConnectionState state_;
  std::mutex socket_mutex_;

  Logger *logger;

  void updateBitfield(const uint32_t pieceIndex);

  std::vector<std::byte> createHandshakeMessage() const;
  std::vector<std::byte> createInterestedMessage() const;
  bool
  isValidHandshake(const std::shared_ptr<boost::array<std::byte, 68>> &buffer);
  void handleConnect(const boost::system::error_code &error);
  void handleHandshakeWrite(const boost::system::error_code &error);
  void handleHandshakeRead(const boost::system::error_code &error,
                           size_t bytes_transferred,
                           std::shared_ptr<boost::array<std::byte, 68>> buffer);
  void processHandshake(const std::vector<std::byte> &response);
  void handleInterestWrite(const boost::system::error_code &error);
  // Express interest in downloading
  void sendInterest();
  // Set the active status of the connection
  void setActive(const bool active);
  void processMessage(const Message message);
  void handleRead(const boost::system::error_code &error,
                  std::size_t bytes_transferred);

  void readMessage();
};

#endif // PEERCONNECTION_H
