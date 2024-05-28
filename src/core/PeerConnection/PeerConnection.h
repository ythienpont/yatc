#ifndef PEERCONNECTION_H
#define PEERCONNECTION_H

#include "FileManager/FileManager.h"
#include "Message/Message.h"
#include "Peer/Peer.h"
#include "PieceManager/PieceManager.h"
#include "Torrent/Torrent.h"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <memory>
#include <unordered_map>

using namespace boost::placeholders;
using tcp = boost::asio::ip::tcp;

// Constants
const int HANDSHAKE_SIZE = 68;
const int PIECE_REQUEST_SIZE = 17;
const int BLOCK_SIZE = 16 * 1024; // 16 KiB block size
const int MAX_CONCURRENT_BLOCK_REQUESTS = 4;

struct ConnectionState {
  bool interested = false;
  bool choked = true;
};

class PieceDownloadState {
public:
  uint32_t piece_index;
  uint32_t total_blocks;
  std::vector<bool> blocks_received;
  std::vector<std::byte> piece_data_buffer; // Buffer to store the entire piece
  uint32_t next_block_to_request;

  PieceDownloadState()
      : piece_index(0), total_blocks(0), piece_data_buffer(0),
        next_block_to_request(0) {} // Default constructor
  PieceDownloadState(uint32_t piece_index, uint32_t total_blocks,
                     uint32_t piece_size)
      : piece_index(piece_index), total_blocks(total_blocks),
        blocks_received(total_blocks, false),
        piece_data_buffer(
            piece_size), // Initialize the buffer with the piece size
        next_block_to_request(0) {}
};

class PeerConnection : public std::enable_shared_from_this<PeerConnection> {
public:
  PeerConnection(boost::asio::io_context &io_context, const InfoHash &info_hash,
                 const Peer::Id &peer_id,
                 std::shared_ptr<PieceManager> piece_manager,
                 std::shared_ptr<LinuxFileManager> file_manager_)
      : socket_(io_context), info_hash_(info_hash), peer_id_(peer_id),
        piece_manager_(piece_manager), file_manager_(file_manager_) {}

  tcp::socket &socket();

  void start();
  void stop();

private:
  void handle_handshake(const boost::system::error_code &error);
  void
  handle_handshake_response(std::shared_ptr<std::vector<std::byte>> response,
                            const boost::system::error_code &error);
  void send_interested();
  void handle_interested(const boost::system::error_code &error);

  void request_piece();
  void send_block_request(uint32_t piece_index, uint32_t block_index);
  void request_more_blocks(PieceDownloadState &piece_request);
  void handle_piece_request(const boost::system::error_code &error);

  void read_message();
  void handle_read_length(const boost::system::error_code &error,
                          std::size_t bytes_transferred);
  void handle_read_message(const boost::system::error_code &error,
                           std::size_t bytes_transferred);
  void process_message(const Message &message);

  void handle_bitfield(const std::vector<std::byte> &bitfield);
  void handle_piece(const PieceData &piece_data);

  tcp::socket socket_;
  InfoHash info_hash_;
  Peer::Id peer_id_;
  std::vector<std::byte> read_buffer_;
  ConnectionState local_state_;
  ConnectionState remote_state_;
  std::vector<bool> bitfield_;
  std::shared_ptr<PieceManager> piece_manager_;
  std::shared_ptr<LinuxFileManager> file_manager_;
  bool request_pending_ = false;
  std::unordered_map<uint32_t, PieceDownloadState> piece_download_states_;
};

#endif // PEERCONNECTION_H
