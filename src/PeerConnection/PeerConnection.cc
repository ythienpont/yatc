#include "PeerConnection.h"

std::vector<std::byte> PeerConnection::constructHandshakeMessage() {
  std::vector<std::byte> handshake;
  handshake.reserve(68); // 1 + 19 + 8 + 20 + 20

  // pstrlen
  handshake.push_back(std::byte{19}); // pstr

  const char *pstr = "BitTorrent protocol";
  for (size_t i = 0; i < 19; ++i) {
    handshake.push_back(static_cast<std::byte>(pstr[i]));
  }

  // 8 reserved bytes
  for (int i = 0; i < 8; ++i) {
    handshake.push_back(std::byte{0});
  }

  // infoHash
  handshake.insert(handshake.end(), infoHash_.begin(), infoHash_.end());

  // peerId
  handshake.insert(handshake.end(), peer_.id.begin(), peer_.id.end());

  return handshake;
}

void PeerConnection::handshake() {
  tcp::resolver resolver(ioContext_);
  auto endpoints = resolver.resolve(peer_.ip, std::to_string(peer_.port));
  boost::asio::async_connect(socket_, endpoints,
                             boost::bind(&PeerConnection::handle_connect, this,
                                         boost::asio::placeholders::error));
}

void PeerConnection::handle_connect(const boost::system::error_code &error) {
  if (!error) {
    writeBuffer_ = constructHandshakeMessage();
    boost::asio::async_write(socket_, boost::asio::buffer(writeBuffer_),
                             boost::bind(&PeerConnection::handle_write, this,
                                         boost::asio::placeholders::error));
  } else {
    std::cerr << "Connect error: " << error.message() << std::endl;
  }
}

void PeerConnection::handle_write(const boost::system::error_code &error) {
  if (!error) {
    // Handshake sent, now read response
    boost::asio::async_read(
        socket_, boost::asio::buffer(readBuffer_),
        boost::bind(&PeerConnection::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
  } else {
    std::cerr << "Write error: " << error.message() << std::endl;
  }
}

void PeerConnection::handle_read(const boost::system::error_code &error,
                                 size_t bytes_transferred) {
  if (!error) {
    // Response received, process it
    // ...
    std::cout << bytes_transferred << std::endl;
  } else {
    std::cerr << "Read error: " << error.message() << std::endl;
  }
}
