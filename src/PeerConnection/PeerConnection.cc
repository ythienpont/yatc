#include "PeerConnection.h"
#include "Message/Message.h"
#include <cstddef>
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
    auto buffer = std::make_shared<boost::array<std::byte, 68>>();
    boost::asio::async_read(socket_, boost::asio::buffer(*buffer),
                            [this, buffer](const boost::system::error_code &ec,
                                           std::size_t bytes_transferred) {
                              handleHandshakeRead(ec, bytes_transferred,
                                                  buffer);
                            });
  } else {
    std::cerr << "Write error: " << error.message() << std::endl;
    setActive(false);
  }
}

void PeerConnection::handleHandshakeRead(
    const boost::system::error_code &error, size_t bytes_transferred,
    std::shared_ptr<boost::array<std::byte, 68>> buffer) {
  if (!error && bytes_transferred == 68) {
    std::string response(reinterpret_cast<char *>(buffer->data()),
                         bytes_transferred);

    if (isValidHandshake(buffer)) {
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

bool PeerConnection::isValidHandshake(
    const std::shared_ptr<boost::array<std::byte, 68>> &buffer) const {
  if (std::to_integer<int>((*buffer)[0]) != 19) {
    std::cerr << "Invalid protocol string length." << std::endl;
    return false;
  }

  const char *expectedPstr = "BitTorrent protocol";
  for (size_t i = 0; i < 19; ++i) {
    if (std::to_integer<char>((*buffer)[1 + i]) != expectedPstr[i]) {
      std::cerr << "Protocol mismatch." << std::endl;
      return false;
    }
  }

  auto receivedInfoHash =
      std::vector<std::byte>(buffer->begin() + 28, buffer->begin() + 48);
  if (!std::equal(infoHash_.begin(), infoHash_.end(),
                  receivedInfoHash.begin())) {
    std::cerr << "Info hash does not match." << std::endl;
    return false;
  }

  auto receivedPeerId =
      std::vector<std::byte>(buffer->begin() + 48, buffer->end());
  if (!std::equal(peer_.id.begin(), peer_.id.end(), receivedPeerId.begin())) {
    std::cerr << "Peer ID does not match." << std::endl;
    return false;
  }

  return true;
}
/*
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
          std::cout << "Interest message sent successfully." << std::endl;
          readMessage();
        }
      });
}
*/

void PeerConnection::sendInterest() {
  const std::vector<char> interestedMsg = {
      0, 0, 0, 1, 2}; // length prefix + message ID for 'interested'
  boost::asio::async_write(
      socket_, boost::asio::buffer(interestedMsg),
      [this](const boost::system::error_code &error,
             std::size_t bytes_transferred) {
        if (!error) {
          std::cout << "Interest message sent successfully." << std::endl;
          readMessage();
        } else {
          std::cerr << "Failed to send interest message: " << error.message()
                    << std::endl;
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

bool PeerConnection::isConnected() const { return isActive_.load(); }

void PeerConnection::setActive(const bool active) { isActive_.store(active); }

void PeerConnection::processMessage(const Message message) {
  // Use std::visit to handle different payload types
  std::visit(
      overloaded{[&](const std::monostate &) {
                   // Handle simple messages that do not have a payload
                   switch (message.type) {
                   case MessageType::Choke:
                     state_.remoteChoked = true;
                     std::cout << "Remote peer choked us." << std::endl;
                     break;
                   case MessageType::Unchoke:
                     state_.remoteChoked = false;
                     std::cout << "Remote peer unchoked us." << std::endl;
                     break;
                   case MessageType::Interested:
                     state_.remoteInterested = true;
                     std::cout << "Remote peer is interested." << std::endl;
                     break;
                   case MessageType::NotInterested:
                     state_.remoteInterested = false;
                     std::cout << "Remote peer is not interested." << std::endl;
                     break;
                   default:
                     std::cerr << "Unexpected message type with no payload."
                               << std::endl;
                   }
                 },
                 [&](const uint32_t index) {
                   // For Have message
                   if (message.type == MessageType::Have) {
                     std::cout << "Have message: peer has piece index " << index
                               << std::endl;
                     // You could update a bitfield here indicating the peer has
                     // this piece
                   }
                 },
                 [&](const std::vector<std::byte> &data) {
                   // For Bitfield and Piece messages
                   if (message.type == MessageType::Bitfield) {
                     std::cout << "Bitfield message received." << std::endl;
                     // Process bitfield data
                   } else if (message.type == MessageType::Piece) {
                     std::cout << "Piece message received." << std::endl;
                     // Process piece data
                   }
                 },
                 [&](const std::tuple<uint32_t, uint32_t, uint32_t> &request) {
                   // For Request and Cancel messages
                   if (message.type == MessageType::Request) {
                     auto [index, begin, length] = request;
                     std::cout << "Request message: index " << index
                               << ", begin " << begin << ", length " << length
                               << std::endl;
                     // Process file request
                   } else if (message.type == MessageType::Cancel) {
                     auto [index, begin, length] = request;
                     std::cout << "Cancel message: index " << index
                               << ", begin " << begin << ", length " << length
                               << std::endl;
                     // Process cancellation of a request
                   }
                 }},
      message.payload);
}

void PeerConnection::readMessage() {
  socket_.async_read_some(boost::asio::buffer(readBuffer_),
                          [this](const boost::system::error_code &error,
                                 std::size_t bytes_transferred) {
                            handleRead(error, bytes_transferred);
                          });
}

void PeerConnection::handleRead(const boost::system::error_code &error,
                                std::size_t bytes_transferred) {
  if (!error) {
    std::cout << bytes_transferred << std::endl;
    readMessage();
    /*
    // Append the data to the message buffer
    messageBuffer_.insert(messageBuffer_.end(), readBuffer_.begin(),
    readBuffer_.begin() + bytes_transferred);

    if (messageBuffer_.size() < 10000()) {
        // If not enough data, read more
        readMessage();
    } else {
        // Process full message
        std::cout << "Full message received" << std::endl;
        // Process the message (not implemented here)

        // Optionally, start reading the next message
        messageBuffer_.clear();
        readMessage();
    }
*/
  } else {
    std::cerr << "Read error: " << error.message() << std::endl;
  }
}
