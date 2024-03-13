#ifndef PEERCONNECTION_H
#define PEERCONNECTION_H

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/bind/bind.hpp>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace boost::placeholders;
using boost::asio::ip::tcp;

class PeerConnection {
public:
  PeerConnection(boost::asio::io_context &ioContext, const std::string &ip,
                 uint16_t port, const std::string &peerId)
      : ioContext_(ioContext), socket_(ioContext), ip_(ip), port_(port),
        peerId_(peerId) {}

  // Establish connection and perform handshake
  void handshake();

  // Express interest in downloading
  void sendInterest();

  // Request a piece of the file
  void sendRequest(int pieceIndex);

  // Handle receiving a piece of the file
  void receivePiece(int pieceIndex, const std::vector<uint8_t> &pieceData);

  // Close the connection
  void disconnect();

private:
  boost::asio::io_context &ioContext_;
  tcp::socket socket_;
  std::string ip_;
  uint16_t port_;
  std::string peerId_;
  std::vector<bool> pieces_; // Tracking which pieces this peer has
  std::vector<uint8_t> writeBuffer_;
  boost::array<uint8_t, 68> readBuffer_;

  // Stub for updating the piece availability from this peer
  void updatePiecesAvailability();

  // Check if the connection is active
  bool isConnected() const;

  void handle_connect(const boost::system::error_code &error);
  void handle_write(const boost::system::error_code &error);
  void handle_read(const boost::system::error_code &error,
                   size_t bytes_transferred);
};

#endif // PEERCONNECTION_H
