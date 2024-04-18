#include "PeerConnection.h"
#include <boost/date_time/gregorian/greg_year.hpp>
#include <iostream>
#include <string>

bool isCompleteMessage(const std::vector<std::byte> &data) {
  const size_t lengthFieldSize =
      4; // Minimum bytes needed to store message length

  if (data.size() < lengthFieldSize) {
    return false; // Not enough data even for the length field
  }

  // Assume messageLength is the first field
  uint32_t messageLength = 0;
  std::memcpy(&messageLength, data.data(), lengthFieldSize);
  messageLength =
      ntohl(messageLength); // Convert network byte order to host byte order

  return data.size() >= static_cast<size_t>(messageLength) + lengthFieldSize;
}

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
                             boost::bind(&PeerConnection::handleConnect, this,
                                         boost::asio::placeholders::error));
}

void PeerConnection::handleConnect(const boost::system::error_code &error) {
  if (!error) {
    setActive(true); // The status will be set back to false if any steps fail
    writeBuffer_ = constructHandshakeMessage();
    boost::asio::async_write(socket_, boost::asio::buffer(writeBuffer_),
                             boost::bind(&PeerConnection::handleHandshakeWrite,
                                         this,
                                         boost::asio::placeholders::error));
  } else {
    std::cerr << "Connect error: " << error.message() << std::endl;
    setActive(false);
  }
}

void PeerConnection::handleHandshakeWrite(
    const boost::system::error_code &error) {
  if (!error) {
    boost::asio::async_read(
        socket_, boost::asio::buffer(handshakeReadBuffer_),
        boost::bind(&PeerConnection::handleHandshakeRead, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
  } else {
    std::cerr << "Write error: " << error.message() << std::endl;
    setActive(false);
  }
}

void PeerConnection::handleHandshakeRead(const boost::system::error_code &error,
                                         size_t bytes_transferred) {
  if (!error && bytes_transferred == 68) {
    std::string response(reinterpret_cast<char *>(handshakeReadBuffer_.data()),
                         bytes_transferred);

    setActive(verifyHandshake());

    if (!isConnected()) {
      std::cerr << "Handshake failed, disconnecting." << std::endl;
      // disconnect();
    } else {
      std::cout << "Handshake successful: " << response << std::endl;
    }
  } else {
    std::cerr << "Read error: " << error.message() << std::endl;
    setActive(false);
  }
}

void PeerConnection::sendInterest() {
  // Interested message format: <length prefix><message ID>
  // Length prefix = 1 (message ID's size), Message ID = 2 (interested)
  /*
  const std::vector<std::byte> msg{std::byte{0}, std::byte{0}, std::byte{0},
                                   std::byte{1}, std::byte{2}};
  boost::asio::async_write(
      socket_, boost::asio::buffer(msg),
      boost::bind(&PeerConnection::handle_write, this, _1));
  */
}

void PeerConnection::disconnect() {
  if (socket_.is_open()) {
    socket_.close();
  }

  setActive(false);
}

bool starts_with(const std::string &fullString, const std::string &starting) {
  if (fullString.length() >= starting.length()) {
    return fullString.compare(0, starting.length(), starting) == 0;
  }
  return false;
}

bool PeerConnection::verifyHandshake() const {
  if (std::to_integer<int>(handshakeReadBuffer_[0]) != 19) { // Check pstrlen
    std::cerr << "Invalid protocol string length." << std::endl;
    return false;
  }
  const char *expectedPstr = "BitTorrent protocol";
  for (size_t i = 0; i < 19; ++i) {
    if (std::to_integer<char>(handshakeReadBuffer_[1 + i]) != expectedPstr[i]) {
      std::cerr << "Protocol mismatch." << std::endl;
      return false;
    }
  }
  // Check the info hash
  auto receivedInfoHash = std::vector<std::byte>(
      handshakeReadBuffer_.begin() + 28, handshakeReadBuffer_.begin() + 48);
  if (!std::equal(infoHash_.begin(), infoHash_.end(),
                  receivedInfoHash.begin())) {
    std::cerr << "Info hash does not match." << std::endl;
    return false;
  }

  // Optionally, check the peer ID if necessary
  auto receivedPeerId = std::vector<std::byte>(
      handshakeReadBuffer_.begin() + 48, handshakeReadBuffer_.end());
  if (!std::equal(peer_.id.begin(), peer_.id.end(), receivedPeerId.begin())) {
    std::cerr << "Peer ID does not match." << std::endl;
    return false;
  }

  // If all checks pass
  return true;
}

bool PeerConnection::isConnected() const { return isActive_.load(); }

void PeerConnection::setActive(const bool active) { isActive_.store(active); }
