#ifndef PEERCONNECTION_H
#define PEERCONNECTION_H

#include "Message/Message.h"
#include "Peer/Peer.h"
#include "PieceManager/PieceManager.h"
#include "Torrent/Torrent.h"
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/bind/bind.hpp>
#include <memory>

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

class PeerConnection : public std::enable_shared_from_this<PeerConnection> {
public:
  PeerConnection(boost::asio::io_context &io_context, const InfoHash &info_hash,
                 const Peer::Id &peer_id,
                 std::shared_ptr<PieceManager> piece_manager)
      : socket_(io_context), info_hash_(info_hash), peer_id_(peer_id),
        piece_manager_(piece_manager) {}

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
  void handle_unchoke(std::vector<std::byte> response,
                      const boost::system::error_code &error);

  void request_piece();
  void send_request(uint32_t piece_index);
  void handle_piece_request(const boost::system::error_code &error);

  void read_message();
  void handle_read(const boost::system::error_code &error,
                   std::size_t bytes_transferred);
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
  bool request_pending_ = false;
};

#endif // PEERCONNECTION_H
