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
    std::string response;
    response.reserve(
        bytes_transferred); // Reserve space to avoid multiple allocations
    for (size_t i = 0; i < bytes_transferred; ++i) {
      response.push_back(static_cast<char>(readBuffer_[i]));
    }

    std::cout << "Response received: " << response << std::endl;

    std::cout << bytes_transferred << std::endl;
  } else {
    std::cerr << "Read error: " << error.message() << std::endl;
  }
}

void PeerConnection::sendInterest() {
  // Interested message format: <length prefix><message ID>
  // Length prefix = 1 (message ID's size), Message ID = 2 (interested)
  const std::vector<std::byte> msg{std::byte{0}, std::byte{0}, std::byte{0},
                                   std::byte{1}, std::byte{2}};
  boost::asio::async_write(
      socket_, boost::asio::buffer(msg),
      boost::bind(&PeerConnection::handle_write, this, _1));
}

void PeerConnection::disconnect() {
  if (socket_.is_open()) {
    socket_.close();
  }
}
