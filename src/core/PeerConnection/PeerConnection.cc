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

      /*if (message_length > 16 * 1024) { // Check for a sane message length
        std::cerr << "Invalid message length: " << message_length << std::endl;
        stop();
        return;
      }*/

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

  if (missing_pieces.size() == 0)
    stop();

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
    uint32_t piece_size = piece_manager_->piece_size(piece_index);
    uint32_t total_blocks = (piece_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    piece_download_states_.emplace(
        piece_index, PieceDownloadState(piece_index, total_blocks, piece_size));

    request_more_blocks(piece_download_states_[piece_index]);
    request_pending_ = true;
    break;
  }
}

void PeerConnection::send_block_request(uint32_t piece_index,
                                        uint32_t block_index) {
  auto self(shared_from_this());

  uint32_t begin = block_index * BLOCK_SIZE;
  uint32_t length = BLOCK_SIZE;

  uint32_t piece_length = piece_manager_->piece_size(piece_index);

  // Adjust length for the last block in the piece if necessary
  if (begin + length > piece_length) {
    length = piece_length - begin;
  }

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

  std::cout << "Sending block request for piece index " << piece_index
            << ", block index " << block_index << ", begin offset " << begin
            << ", block length " << length << std::endl;

  boost::asio::async_write(socket_,
                           boost::asio::buffer(request, PIECE_REQUEST_SIZE),
                           boost::bind(&PeerConnection::handle_piece_request,
                                       self, boost::asio::placeholders::error));
}

void PeerConnection::request_more_blocks(PieceDownloadState &piece_request) {
  uint32_t blocks_to_request = 0;
  while (blocks_to_request < MAX_CONCURRENT_BLOCK_REQUESTS &&
         piece_request.next_block_to_request < piece_request.total_blocks) {
    if (!piece_request.blocks_received[piece_request.next_block_to_request]) {
      send_block_request(piece_request.piece_index,
                         piece_request.next_block_to_request);
      blocks_to_request++;
    }
    piece_request.next_block_to_request++;
  }
}

void PeerConnection::handle_piece_request(
    const boost::system::error_code &error) {
  if (!error) {
    std::cout << "Piece request sent successfully." << std::endl;
    request_pending_ = false;
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
            << ", Begin: " << piece_data.begin
            << ", Block size: " << piece_data.block.size() << std::endl;

  // Find the matching piece request
  auto it = piece_download_states_.find(piece_data.index);

  if (it != piece_download_states_.end()) {
    PieceDownloadState &piece_request = it->second;
    uint32_t block_offset = piece_data.begin;

    std::cout << "Storing block at offset: " << block_offset << std::endl;

    // Store the received block in the buffer
    std::copy(piece_data.block.begin(), piece_data.block.end(),
              piece_request.piece_data_buffer.begin() + block_offset);

    // Calculate block index based on the offset
    uint32_t block_index = piece_data.begin / BLOCK_SIZE;
    piece_request.blocks_received[block_index] = true;

    // Print the blocks received so far for debugging
    std::cout << "Blocks received for piece " << piece_data.index << ": ";
    for (const auto &received : piece_request.blocks_received) {
      std::cout << received << " ";
    }
    std::cout << std::endl;

    // Check if all blocks for this piece are received
    if (std::all_of(piece_request.blocks_received.begin(),
                    piece_request.blocks_received.end(),
                    [](bool received) { return received; })) {
      std::cout << "All blocks for piece " << piece_data.index << " received."
                << std::endl;

      // Save the complete piece if file_manager_ is not null
      if (file_manager_) {
        file_manager_->write_piece(piece_data.index,
                                   piece_request.piece_data_buffer);
        std::cout << "Piece " << piece_data.index << " written to file."
                  << std::endl;
        piece_manager_->save_piece(piece_data.index);
      } else {
        std::cerr << "Error: file_manager_ is null. Cannot write piece "
                  << piece_data.index << std::endl;
      }
      piece_download_states_.erase(it);
    } else {
      // Request the next set of blocks
      request_more_blocks(piece_request);
      std::cout << "Requesting more blocks for piece " << piece_data.index
                << std::endl;
    }
  } else {
    std::cerr << "Error: No matching piece request found for index "
              << piece_data.index << std::endl;
  }

  request_pending_ = false;
}
