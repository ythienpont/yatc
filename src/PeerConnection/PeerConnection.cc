#include "PeerConnection.h"
#include "Logger/Logger.h"
#include "Message/Message.h"
#include <cstddef>
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
  handshake.insert(handshake.end(), myPeerId_.begin(), myPeerId_.end());

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
    Logger::instance()->log("Connect error: " + error.message(), Logger::ERROR);
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
    Logger::instance()->log("Write error: " + error.message(), Logger::ERROR);
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
      Logger::instance()->log("Handshake successful: " + response,
                              Logger::INFO);
      sendInterest();
    } else {
      Logger::instance()->log("Handshake failed, disconnecting.",
                              Logger::ERROR);
      setActive(false);
      closeConnection();
    }
  } else {
    Logger::instance()->log("Read error: " + error.message(), Logger::ERROR);
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
    const std::shared_ptr<boost::array<std::byte, 68>> &buffer) {
  if (std::to_integer<int>((*buffer)[0]) != 19) {
    Logger::instance()->log("Invalid protocol string length.", Logger::ERROR);
    return false;
  }

  const char *expectedPstr = "BitTorrent protocol";
  for (size_t i = 0; i < 19; ++i) {
    if (std::to_integer<char>((*buffer)[1 + i]) != expectedPstr[i]) {
      Logger::instance()->log("Protocol mismatch.", Logger::ERROR);
      return false;
    }
  }

  auto receivedInfoHash =
      std::vector<std::byte>(buffer->begin() + 28, buffer->begin() + 48);
  if (!std::equal(infoHash_.begin(), infoHash_.end(),
                  receivedInfoHash.begin())) {
    Logger::instance()->log("Info hash does not match.", Logger::ERROR);
    return false;
  }

  auto receivedPeerId =
      std::vector<std::byte>(buffer->begin() + 48, buffer->end());

  if (peer_.isIdSet()) {
    if (!std::equal(peer_.id->begin(), peer_.id->end(),
                    receivedPeerId.begin())) {
      Logger::instance()->log("Peer ID does not match.", Logger::ERROR);
      return false;
    }
  } else {
    std::array<std::byte, 20> peerIdArray;
    std::copy(receivedPeerId.begin(), receivedPeerId.end(),
              peerIdArray.begin());
    peer_.id = peerIdArray;
  }

  return true;
}

void PeerConnection::sendInterest() {
  const std::vector<char> interestedMsg = {
      0, 0, 0, 1, 2}; // length prefix + message ID for 'interested'
  boost::asio::async_write(
      socket_, boost::asio::buffer(interestedMsg),
      [this](const boost::system::error_code &error,
             std::size_t bytes_transferred) {
        if (!error) {
          Logger::instance()->log("Interest message sent successfully.",
                                  Logger::INFO);
          readMessage();
        } else {
          Logger::instance()->log("Failed to send interest message: " +
                                      error.message(),
                                  Logger::ERROR);
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
      overloaded{
          [&](const std::monostate &) {
            // Handle simple messages that do not have a payload
            switch (message.type) {
            case MessageType::Choke:
              state_.remoteChoked = true;
              Logger::instance()->log("Remote peer choked us.", Logger::INFO);
              break;
            case MessageType::Unchoke:
              state_.remoteChoked = false;
              Logger::instance()->log("Remote peer unchoked us.", Logger::INFO);
              break;
            case MessageType::Interested:
              state_.remoteInterested = true;
              Logger::instance()->log("Remote peer is interested.",
                                      Logger::INFO);
              break;
            case MessageType::NotInterested:
              state_.remoteInterested = false;
              Logger::instance()->log("Remote peer is not interested.",
                                      Logger::INFO);
              break;
            default:
              Logger::instance()->log(
                  "Unexpected message type with no payload.", Logger::ERROR);
            }
          },
          [&](const uint32_t index) {
            // For Have message
            if (message.type == MessageType::Have) {
              Logger::instance()->log("Have message: peer has piece index " +
                                          std::to_string(index),
                                      Logger::INFO);
              // You could update a bitfield here indicating the peer has
              // this piece
            }
          },
          [&](const std::vector<std::byte> &data) {
            // For Bitfield and Piece messages
            if (message.type == MessageType::Bitfield) {
              Logger::instance()->log("Bitfield message received.",
                                      Logger::INFO);
              // Process bitfield data
            } else if (message.type == MessageType::Piece) {
              Logger::instance()->log("Piece message received.", Logger::INFO);
              // Process piece data
            }
          },
          [&](const std::tuple<uint32_t, uint32_t, uint32_t> &request) {
            // For Request and Cancel messages
            if (message.type == MessageType::Request) {
              auto [index, begin, length] = request;
              Logger::instance()->log("Request message: index " +
                                          std::to_string(index) + ", begin " +
                                          std::to_string(begin) + ", length " +
                                          std::to_string(length),
                                      Logger::INFO);
              // Process file request
            } else if (message.type == MessageType::Cancel) {
              auto [index, begin, length] = request;
              Logger::instance()->log("Cancel message: index " +
                                          std::to_string(index) + ", begin " +
                                          std::to_string(begin) + ", length " +
                                          std::to_string(length),
                                      Logger::INFO);
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
    Logger::instance()->log("Bytes transferred: " +
                                std::to_string(bytes_transferred),
                            Logger::DEBUG);

    // Append received data to the message buffer
    messageBuffer_.insert(messageBuffer_.end(), readBuffer_.begin(),
                          readBuffer_.begin() + bytes_transferred);

    // Log the current state of the message buffer
    Logger::instance()->log("Current message buffer size: " +
                                std::to_string(messageBuffer_.size()),
                            Logger::DEBUG);

    // Continuously process messages while there's enough data in the buffer
    while (messageBuffer_.size() >= sizeof(uint32_t)) {
      // Extract and convert the message length from network byte order to host
      // byte order
      uint32_t rawMessageLength =
          *reinterpret_cast<const uint32_t *>(messageBuffer_.data());
      uint32_t messageLength = ntohl(rawMessageLength);
      Logger::instance()->log("Raw message length (network order): " +
                                  std::to_string(rawMessageLength),
                              Logger::DEBUG);
      Logger::instance()->log("Message length (host order): " +
                                  std::to_string(messageLength),
                              Logger::DEBUG);

      if (messageLength == 0) {
        // Keep-alive message (length prefix of zero)
        Logger::instance()->log("Keep-alive message received.", Logger::INFO);
        messageBuffer_.erase(messageBuffer_.begin(),
                             messageBuffer_.begin() + sizeof(uint32_t));
        continue; // Continue to check for more messages in the buffer
      }

      // Calculate the total message length (payload length + size of the length
      // field itself)
      uint32_t totalMessageLength = messageLength + sizeof(uint32_t);

      // Check if the complete message has been received
      if (messageBuffer_.size() >= totalMessageLength) {
        std::vector<std::byte> completeMessage(
            messageBuffer_.begin() + sizeof(uint32_t),
            messageBuffer_.begin() + totalMessageLength);

        // Log the received message content
        std::ostringstream msgContent;
        for (auto byte : completeMessage) {
          msgContent << std::to_integer<int>(byte) << " ";
        }
        Logger::instance()->log(
            "Complete message received: " + msgContent.str(), Logger::DEBUG);

        try {
          Message msg = Message::parseMessage(completeMessage);
          processMessage(msg);

          // Erase processed message from buffer
          messageBuffer_.erase(messageBuffer_.begin(),
                               messageBuffer_.begin() + totalMessageLength);
          Logger::instance()->log("Message processed and erased from buffer.",
                                  Logger::DEBUG);
        } catch (const std::runtime_error &e) {
          Logger::instance()->log(
              "Message parsing error: " + std::string(e.what()), Logger::ERROR);
          // Break out of the loop if there is an error to avoid infinite loops
          break;
        }
      } else {
        // Not enough data received yet, wait for more data
        Logger::instance()->log(
            "Incomplete message received, waiting for more data.",
            Logger::DEBUG);
        break;
      }
    }

    // Continue reading
    readMessage();
  } else {
    Logger::instance()->log("Read error: " + error.message(), Logger::ERROR);
  }
}
