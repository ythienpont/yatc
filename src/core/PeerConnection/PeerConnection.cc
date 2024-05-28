#include "PeerConnection.h"
#include "Message/Message.h"
#include "Torrent/Torrent.h"
#include <boost/asio.hpp>
#include <iostream>
#include <unordered_set>
#include <vector>

std::vector<std::byte> create_handshake(const InfoHash &info_hash,
                                        const Peer::Id &peer_id) {
  std::vector<std::byte> handshake;
  handshake.reserve(HANDSHAKE_SIZE); // 1 + 19 + 8 + 20 + 20

  // pstrlen
  handshake.push_back(std::byte{19});

  // pstr
  const char *protocol_string = "BitTorrent protocol";
  for (size_t i = 0; i < 19; ++i) {
    handshake.push_back(static_cast<std::byte>(protocol_string[i]));
  }

  // 8 reserved bytes
  for (int i = 0; i < 8; ++i) {
    handshake.push_back(std::byte{0});
  }

  // infoHash
  handshake.insert(handshake.end(), info_hash.begin(), info_hash.end());

  // peerId
  handshake.insert(handshake.end(), peer_id.begin(), peer_id.end());

  return handshake;
}

tcp::socket &PeerConnection::socket() { return socket_; }

void PeerConnection::start() {
  auto self(shared_from_this());
  std::vector<std::byte> handshake = create_handshake(info_hash_, peer_id_);
  boost::asio::async_write(socket_, boost::asio::buffer(handshake),
                           boost::bind(&PeerConnection::handle_handshake, self,
                                       boost::asio::placeholders::error));
}

void PeerConnection::stop() {
  if (socket_.is_open()) {
    socket_.close();
  }
}

void PeerConnection::handle_handshake(const boost::system::error_code &error) {
  if (!error) {
    auto self(shared_from_this());
    auto response = std::make_shared<std::vector<std::byte>>(HANDSHAKE_SIZE);
    boost::asio::async_read(
        socket_, boost::asio::buffer(*response),
        boost::bind(&PeerConnection::handle_handshake_response, self, response,
                    boost::asio::placeholders::error));
  } else {
    stop();
  }
}

void PeerConnection::handle_handshake_response(
    std::shared_ptr<std::vector<std::byte>> response,
    const boost::system::error_code &error) {
  if (!error) {
    send_interested();
  } else {
    stop();
  }
}

void PeerConnection::send_interested() {
  auto self(shared_from_this());
  std::vector<std::byte> interested = {std::byte{0x00}, std::byte{0x00},
                                       std::byte{0x00}, std::byte{0x01},
                                       std::byte{0x02}};
  boost::asio::async_write(socket_,
                           boost::asio::buffer(interested, interested.size()),
                           boost::bind(&PeerConnection::handle_interested, self,
                                       boost::asio::placeholders::error));
}

void PeerConnection::handle_interested(const boost::system::error_code &error) {
  if (!error) {
    read_message();
  }
}

void PeerConnection::read_message() {
  std::cout << "Reading message" << std::endl;
  auto self(shared_from_this());
  read_buffer_.resize(4);
  boost::asio::async_read(
      socket_, boost::asio::buffer(read_buffer_),
      boost::bind(&PeerConnection::handle_read_length, self,
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

void PeerConnection::handle_read_length(const boost::system::error_code &error,
                                        std::size_t bytes_transferred) {
  if (!error) {
    std::cout << "Read length: " << bytes_transferred << " bytes" << std::endl;
    if (bytes_transferred == 4) {
      uint32_t message_length =
          ntohl(*reinterpret_cast<uint32_t *>(read_buffer_.data()));
      std::cout << "Message length: " << message_length << std::endl;

      if (message_length == 0) {
        std::cout << "Keep-alive message received" << std::endl;
        read_message();
        return;
      }

      if (message_length > 16 * 1024) { // Check for a sane message length
        std::cerr << "Invalid message length: " << message_length << std::endl;
        stop();
        return;
      }

      read_buffer_.resize(4 + message_length);
      auto self(shared_from_this());
      boost::asio::async_read(
          socket_, boost::asio::buffer(&read_buffer_[4], message_length),
          boost::bind(&PeerConnection::handle_read_message, self,
                      boost::asio::placeholders::error,
                      boost::asio::placeholders::bytes_transferred));
    } else {
      std::cerr << "Error: Expected 4 bytes for message length, but got "
                << bytes_transferred << std::endl;
      read_message();
    }
  } else {
    std::cerr << "Error reading message length: " << error.message()
              << std::endl;
    stop();
  }
}

void PeerConnection::handle_read_message(const boost::system::error_code &error,
                                         std::size_t bytes_transferred) {
  if (!error) {
    std::cout << "Read message: " << bytes_transferred << " bytes" << std::endl;
    if (bytes_transferred == read_buffer_.size() - 4) {
      // Process the complete message
      Message message = Message::parseMessage(read_buffer_);
      std::cout << "Message parsed: Type = " << static_cast<int>(message.type)
                << std::endl;
      process_message(message);

      // Clear the buffer for the next message
      read_buffer_.clear();

      // Start reading the next message
      read_message();
    } else {
      std::cerr << "Error: Expected " << (read_buffer_.size() - 4)
                << " bytes, but got " << bytes_transferred << std::endl;
      read_message();
    }
  } else {
    std::cerr << "Error reading message: " << error.message() << std::endl;
    stop();
  }
}

void PeerConnection::process_message(const Message &message) {
  std::cout << "Processing message of type: " << static_cast<int>(message.type)
            << std::endl;

  auto handle_payload = overloaded{
      [](std::monostate) {}, [](uint32_t piece_index) {},
      [this](const std::vector<std::byte> &bitfield_data) {
        handle_bitfield(bitfield_data);
      },
      [](const std::tuple<uint32_t, uint32_t, uint32_t> &request_data) {},
      [this](const PieceData &piece_data) { handle_piece(piece_data); }};

  switch (message.type) {
  case MessageType::Choke:
    std::cout << "Choked by peer" << std::endl;
    local_state_.choked = true;
    break;
  case MessageType::Unchoke:
    std::cout << "Unchoked by peer" << std::endl;
    local_state_.choked = false;
    break;
  case MessageType::Interested:
    std::cout << "Remote peer is interested" << std::endl;
    remote_state_.interested = true;
    break;
  case MessageType::NotInterested:
    std::cout << "Remote peer is not interested" << std::endl;
    remote_state_.interested = false;
    break;
  case MessageType::Have:
    std::cout << "Have message received" << std::endl;
    std::visit(handle_payload, message.payload);
    break;
  case MessageType::Bitfield:
    std::cout << "Bitfield received" << std::endl;
    std::visit(handle_payload, message.payload);
    break;
  case MessageType::Request:
    std::cout << "Request message received" << std::endl;
    std::visit(handle_payload, message.payload);
    break;
  case MessageType::Piece:
    std::cout << "Piece message received" << std::endl;
    std::visit(handle_payload, message.payload);
    break;
  case MessageType::Cancel:
    std::cout << "Cancel message received" << std::endl;
    std::visit(handle_payload, message.payload);
    break;
  default:
    std::cerr << "Unknown message type received: "
              << static_cast<int>(message.type) << std::endl;
    break;
  }

  if (!local_state_.choked && !request_pending_) {
    request_piece();
  }
}

void PeerConnection::request_piece() {
  std::unordered_set<uint32_t> missing_pieces =
      piece_manager_->missing_pieces();

  std::cout << "Missing pieces: ";
  for (const auto &piece_index : missing_pieces) {
    std::cout << piece_index << " ";
  }
  std::cout << std::endl;

  for (const auto &piece_index : missing_pieces) {
    if (piece_index >= bitfield_.size()) {
      std::cerr << "Piece index " << piece_index << " is out of bounds."
                << std::endl;
      continue;
    }

    if (!bitfield_[piece_index]) {
      std::cout << "Peer doesn't have piece " << piece_index << std::endl;
      continue;
    }

    std::cout << "Requesting piece index " << piece_index << std::endl;
    send_request(piece_index);
    request_pending_ = true;
    break;
  }
}

void PeerConnection::send_request(uint32_t piece_index) {
  auto self(shared_from_this());

  // Example offset and length for the request
  uint32_t begin = 0;   // Start of the piece
  uint32_t length = 43; // Length of the block to request

  std::vector<std::byte> request(PIECE_REQUEST_SIZE, std::byte{0});
  // Length of the request message (13 bytes)
  request[0] = std::byte{0x00};
  request[1] = std::byte{0x00};
  request[2] = std::byte{0x00};
  request[3] = std::byte{0x0d}; // Length of the request message
  request[4] = std::byte{0x06}; // Request message ID

  // Piece index
  request[5] = static_cast<std::byte>((piece_index >> 24) & 0xFF);
  request[6] = static_cast<std::byte>((piece_index >> 16) & 0xFF);
  request[7] = static_cast<std::byte>((piece_index >> 8) & 0xFF);
  request[8] = static_cast<std::byte>(piece_index & 0xFF);

  // Begin offset
  request[9] = static_cast<std::byte>((begin >> 24) & 0xFF);
  request[10] = static_cast<std::byte>((begin >> 16) & 0xFF);
  request[11] = static_cast<std::byte>((begin >> 8) & 0xFF);
  request[12] = static_cast<std::byte>(begin & 0xFF);

  // Block length
  request[13] = static_cast<std::byte>((length >> 24) & 0xFF);
  request[14] = static_cast<std::byte>((length >> 16) & 0xFF);
  request[15] = static_cast<std::byte>((length >> 8) & 0xFF);
  request[16] = static_cast<std::byte>(length & 0xFF);

  std::cout << "Sending request for piece index " << piece_index
            << ", begin offset " << begin << ", block length " << length
            << std::endl;

  boost::asio::async_write(socket_,
                           boost::asio::buffer(request, PIECE_REQUEST_SIZE),
                           boost::bind(&PeerConnection::handle_piece_request,
                                       self, boost::asio::placeholders::error));
}

void PeerConnection::handle_piece_request(
    const boost::system::error_code &error) {
  if (!error) {
    std::cout << "Piece request sent successfully." << std::endl;
  } else {
    std::cerr << "Piece request error: " << error.message() << std::endl;
  }
}

void PeerConnection::handle_bitfield(
    const std::vector<std::byte> &bitfield_data) {
  bitfield_.clear();
  for (const auto &byte : bitfield_data) {
    for (int i = 7; i >= 0; --i) {
      bitfield_.push_back(
          static_cast<bool>(std::to_integer<int>(byte) & (1 << i)));
    }
  }
}

void PeerConnection::handle_piece(const PieceData &piece_data) {
  std::cout << "Handling piece data - Index: " << piece_data.index
            << ", Block size: " << piece_data.block.size() << std::endl;

  // Save the piece data using the piece manager
  piece_manager_->save_piece(piece_data.index);

  // Clear the request pending flag
  request_pending_ = false;
}
