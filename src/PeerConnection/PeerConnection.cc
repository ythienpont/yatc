#include "PeerConnection.h"
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

std::vector<std::byte> PeerConnection::createHandshakeMessage() const {
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
  std::lock_guard<std::mutex> lock(socket_mutex_);
  tcp::resolver resolver(ioContext_);
  auto endpoints = resolver.resolve(peer_.ip, std::to_string(peer_.port));

  boost::asio::async_connect(socket_, endpoints,
                             boost::bind(&PeerConnection::handleConnect, this,
                                         boost::asio::placeholders::error));
}

void PeerConnection::handleConnect(const boost::system::error_code &error) {
  std::lock_guard<std::mutex> lock(socket_mutex_);
  if (!error) {
    setActive(true); // The status will be set back to false if any steps fail
    writeBuffer_ = createHandshakeMessage();
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
  std::lock_guard<std::mutex> lock(socket_mutex_);
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

    setActive(isValidHandshake());

    if (isValidHandshake()) {
      setActive(true);
      std::cout << "Handshake successful: " << response << std::endl;
      sendInterest();
    } else {
      std::cerr << "Handshake failed, disconnecting." << std::endl;
      setActive(false);
      closeConnection();
    }
  } else {
    std::cerr << "Read error: " << error.message() << std::endl;
    setActive(false);
  }
}

void PeerConnection::closeConnection() {
  std::lock_guard<std::mutex> lock(socket_mutex_);
  if (socket_.is_open()) {
    socket_.close();
  }

  setActive(false);
}

bool PeerConnection::isValidHandshake() const {
  if (std::to_integer<int>(handshakeReadBuffer_[0]) != 19) {
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

  auto receivedInfoHash = std::vector<std::byte>(
      handshakeReadBuffer_.begin() + 28, handshakeReadBuffer_.begin() + 48);
  if (!std::equal(infoHash_.begin(), infoHash_.end(),
                  receivedInfoHash.begin())) {
    std::cerr << "Info hash does not match." << std::endl;
    return false;
  }

  auto receivedPeerId = std::vector<std::byte>(
      handshakeReadBuffer_.begin() + 48, handshakeReadBuffer_.end());
  if (!std::equal(peer_.id.begin(), peer_.id.end(), receivedPeerId.begin())) {
    std::cerr << "Peer ID does not match." << std::endl;
    return false;
  }

  return true;
}

void PeerConnection::sendInterest() {
  std::lock_guard<std::mutex> lock(socket_mutex_);
  std::vector<std::byte> interestedMsg = createInterestedMessage();
  boost::asio::async_write(
      socket_, boost::asio::buffer(interestedMsg),
      [this](const boost::system::error_code &writeError, std::size_t) {
        if (writeError) {
          std::cerr << "Error sending interest message: "
                    << writeError.message() << std::endl;
          setActive(false);
          closeConnection();
        } else {
          waitForUnchoke();
          std::cout << "Interest message sent successfully." << std::endl;
        }
      });
}

void startMainLoop() { std::cout << "IT WORKS" << std::endl; }

void PeerConnection::waitForUnchoke() {
  std::lock_guard<std::mutex> lock(socket_mutex_);
  // Assuming unchoke message is a known size, e.g., 5 bytes for simplicity
  std::vector<std::byte> unchokeBuffer(
      5); // Adjust the size as per the protocol

  boost::asio::async_read(
      socket_, boost::asio::buffer(unchokeBuffer),
      [this, unchokeBuffer](const boost::system::error_code &readError,
                            std::size_t bytes_transferred) mutable {
        if (readError) {
          std::cerr << "Error reading unchoke message: " << readError.message()
                    << std::endl;
          setActive(false);
          closeConnection();
        } else {
          // Assuming the unchoke message has a specific byte pattern; checking
          // it
          if (isUnchokeMessage(unchokeBuffer, bytes_transferred)) {
            std::cout
                << "Unchoke message received, now in the main download loop."
                << std::endl;
            startMainLoop(); // Proceed with main data transfer loop
          } else {
            std::cerr
                << "Expected unchoke message, but received something else."
                << std::endl;
            setActive(false);
            closeConnection();
          }
        }
      });
}

std::vector<std::byte> PeerConnection::createInterestedMessage() const {
  std::vector<std::byte> msg;
  msg.push_back(std::byte{0}); // Length prefix high byte
  msg.push_back(std::byte{0}); // Length prefix mid byte
  msg.push_back(std::byte{0}); // Length prefix low byte
  msg.push_back(std::byte{1}); // Length of 1
  msg.push_back(std::byte{2}); // Message ID for 'interested'

  return msg;
}

bool PeerConnection::isUnchokeMessage(const std::vector<std::byte> &data,
                                      size_t length) const {
  return length == 5 && data[4] == std::byte{0x01};
}

bool PeerConnection::isConnected() const { return isActive_.load(); }

void PeerConnection::setActive(const bool active) { isActive_.store(active); }
void PeerConnection::setInterested(const bool interested) {
  interested_.store(interested);
}
