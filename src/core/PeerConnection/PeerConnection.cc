#include "PeerConnection.h"
#include "Message/Message.h"
#include "Torrent/Torrent.h"
#include <boost/asio.hpp>
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

  // Start handshake
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
    std::cout << "Handshake succesful" << std::endl;
    send_interested_message();
  } else {
    stop();
  }
}

void PeerConnection::send_interested_message() {
  auto self(shared_from_this());
  std::vector<std::byte> interested = {std::byte{0x00}, std::byte{0x00},
                                       std::byte{0x00}, std::byte{0x01},
                                       std::byte{0x02}};
  boost::asio::async_write(
      socket_, boost::asio::buffer(interested, interested.size()),
      boost::bind(&PeerConnection::handle_interested_message, self,
                  boost::asio::placeholders::error));
}

void PeerConnection::handle_interested_message(
    const boost::system::error_code &error) {
  if (!error) {
    read_message();
  }
}

void PeerConnection::read_message() {
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
    // Read the first 4 bytes to get the message length
    if (bytes_transferred == 4) {
      uint32_t message_length =
          ntohl(*reinterpret_cast<uint32_t *>(read_buffer_.data()));

      // If message length is zero, it is a keep-alive message
      if (message_length == 0) {
        read_message();
        return;
      }

      // Read the remaining part of the message
      read_buffer_.resize(4 + message_length);
      auto self(shared_from_this());
      boost::asio::async_read(
          socket_, boost::asio::buffer(&read_buffer_[4], message_length),
          boost::bind(&PeerConnection::handle_read_message, self,
                      boost::asio::placeholders::error,
                      boost::asio::placeholders::bytes_transferred));
    } else {
      read_message();
    }
  } else {
    stop();
  }
}

void PeerConnection::handle_read_message(const boost::system::error_code &error,
                                         std::size_t bytes_transferred) {
  if (!error) {
    if (bytes_transferred == read_buffer_.size() - 4) { // All bytes read
      Message message = Message::parseMessage(read_buffer_);
      process_message(message);

      read_buffer_.clear();
      read_message();
    } else {
      read_message(); // Keep reading
    }
  } else {
    stop();
  }
}

void PeerConnection::process_message(const Message &message) {
  auto handle_payload = overloaded{
      [](std::monostate) {}, [](uint32_t piece_index) {},
      [this](const std::vector<std::byte> &bitfield_data) {
        handle_bitfield_message(bitfield_data);
      },
      [](const std::tuple<uint32_t, uint32_t, uint32_t> &request_data) {},
      [this](const PieceData &piece_data) {
        handle_piece_message(piece_data);
      }};

  switch (message.type) {
  case MessageType::Choke:
    local_state_.choked = true;
    break;
  case MessageType::Unchoke:
    local_state_.choked = false;
    break;
  case MessageType::Interested:
    remote_state_.interested = true;
    break;
  case MessageType::NotInterested:
    remote_state_.interested = false;
    break;
  case MessageType::Have:
    std::visit(handle_payload, message.payload);
    break;
  case MessageType::Bitfield:
    std::visit(handle_payload, message.payload);
    break;
  case MessageType::Request: // TODO
    std::visit(handle_payload, message.payload);
    break;
  case MessageType::Piece:
    std::visit(handle_payload, message.payload);
    break;
  case MessageType::Cancel: // TODO
    std::visit(handle_payload, message.payload);
    break;
  default:
    break;
  }

  if (!local_state_.choked && !request_pending_) {
    request_piece();
  }
}

void PeerConnection::request_piece() {
  // Get the list of missing pieces from the PieceManager
  std::unordered_set<uint32_t> missing_pieces =
      piece_manager_->missing_pieces();

  // If there are no missing pieces, stop the connection
  if (missing_pieces.size() == 0) {
    stop();
    return;
  }

  // Iterate over the missing pieces to request one
  for (const auto &piece_index : missing_pieces) {
    // Skip if the piece index is out of bounds
    if (piece_index >= bitfield_.size()) {
      continue;
    }

    // Skip if the peer doesn't have this piece
    if (!bitfield_[piece_index]) {
      continue;
    }

    // Get the size of the piece and calculate the total number of blocks
    uint32_t piece_size = piece_manager_->piece_size(piece_index);
    uint32_t total_blocks = (piece_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // Initialize the download state for the piece
    piece_download_states_.emplace(
        piece_index, PieceDownloadState(piece_index, total_blocks, piece_size));

    // Request blocks for the piece
    request_more_blocks(piece_download_states_[piece_index]);
    request_pending_ = true;
    break;
  }
}

void PeerConnection::send_block_request(uint32_t piece_index,
                                        uint32_t block_index) {
  auto self(shared_from_this());

  // Calculate the beginning offset and length of the block
  uint32_t begin = block_index * BLOCK_SIZE;
  uint32_t length = BLOCK_SIZE;
  uint32_t piece_length = piece_manager_->piece_size(piece_index);

  // Adjust the length for the last block if necessary
  if (begin + length > piece_length) {
    length = piece_length - begin;
  }

  // Create a block request message
  std::vector<std::byte> request(PIECE_REQUEST_SIZE, std::byte{0});
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

  // Send the block request to the peer
  boost::asio::async_write(
      socket_, boost::asio::buffer(request, PIECE_REQUEST_SIZE),
      boost::bind(&PeerConnection::handle_piece_request_response, self,
                  boost::asio::placeholders::error));
}

void PeerConnection::request_more_blocks(PieceDownloadState &piece_request) {
  uint32_t blocks_to_request = 0;

  // Request more blocks while staying within the limit of concurrent requests
  while (blocks_to_request < MAX_CONCURRENT_BLOCK_REQUESTS &&
         piece_request.next_block_to_request < piece_request.total_blocks) {
    // If the block has not been received, request it
    if (!piece_request.blocks_received[piece_request.next_block_to_request]) {
      send_block_request(piece_request.piece_index,
                         piece_request.next_block_to_request);
      blocks_to_request++;
    }
    piece_request.next_block_to_request++;
  }
}

void PeerConnection::handle_piece_request_response(
    const boost::system::error_code &error) {
  if (!error) {
    request_pending_ = false;
  } else {
    stop();
  }
}

void PeerConnection::handle_bitfield_message(
    const std::vector<std::byte> &bitfield_data) {
  bitfield_.clear();

  // Convert the bitfield data to a vector of booleans
  for (const auto &byte : bitfield_data) {
    for (int i = 7; i >= 0; --i) {
      bitfield_.push_back(
          static_cast<bool>(std::to_integer<int>(byte) & (1 << i)));
    }
  }
}

void PeerConnection::handle_piece_message(const PieceData &piece_data) {
  // Find the matching piece request
  auto it = piece_download_states_.find(piece_data.index);

  if (it != piece_download_states_.end()) {
    PieceDownloadState &piece_request = it->second;
    uint32_t block_offset = piece_data.begin;

    std::copy(piece_data.block.begin(), piece_data.block.end(),
              piece_request.piece_data_buffer.begin() + block_offset);

    uint32_t block_index = piece_data.begin / BLOCK_SIZE;
    piece_request.blocks_received[block_index] = true;

    // Check if all blocks for this piece are received
    if (std::all_of(piece_request.blocks_received.begin(),
                    piece_request.blocks_received.end(),
                    [](bool received) { return received; })) {
      file_manager_->write_piece(piece_data.index,
                                 piece_request.piece_data_buffer);
      // TODO: Check if write succeeds, I'm just happy it works for now
      piece_manager_->save_piece(piece_data.index);

      piece_download_states_.erase(it);
    } else {
      request_more_blocks(piece_request);
    }
  }

  request_pending_ = false;
}
