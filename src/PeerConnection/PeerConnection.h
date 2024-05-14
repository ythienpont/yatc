#ifndef PEERCONNECTION_H
#define PEERCONNECTION_H

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/bind/bind.hpp>
#include <cstdint>
#include <mutex>
#include <vector>

using namespace boost::placeholders;
using tcp = boost::asio::ip::tcp;

struct Peer {
  using Id = std::array<std::byte, 20>;
  Id id;
  std::string ip;
  uint16_t port; // Port at which the torrent service is running
};

class PeerConnection {
public:
  PeerConnection(boost::asio::io_context &ioContext, const Peer peer,
                 const std::array<std::byte, 20> myPeerId,
                 const std::array<std::byte, 20> infoHash)
      : ioContext_(ioContext), socket_(ioContext), peer_(peer),
        myPeerId_(myPeerId), infoHash_(infoHash) {}

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
  boost::array<std::byte, 68> handshakeReadBuffer_;
  std::atomic<bool> isActive_{false};
  std::atomic<bool> interested_{false};
  std::atomic<bool> choked_{true};
  std::mutex socket_mutex_;

  // Stub for updating the piece availability from this peer
  void updatePiecesAvailability();

  std::vector<std::byte> createHandshakeMessage() const;
  std::vector<std::byte> createInterestedMessage() const;
  bool isValidHandshake() const;
  bool isUnchokeMessage(const std::vector<std::byte> &data,
                        size_t length) const;
  void handleConnect(const boost::system::error_code &error);
  void handleHandshakeWrite(const boost::system::error_code &error);
  void handleHandshakeRead(const boost::system::error_code &error,
                           size_t bytes_transferred);
  void processHandshake(const std::vector<std::byte> &response);
  void handleInterestWrite(const boost::system::error_code &error);
  void waitForUnchoke();
  void handleUnchoke(const boost::system::error_code &error,
                     size_t bytes_transferred);
  // Express interest in downloading
  void sendInterest();
  // Set the active status of the connection
  void setActive(const bool active);
  void setInterested(const bool interested);
};

#endif // PEERCONNECTION_H
