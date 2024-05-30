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

const int HANDSHAKE_SIZE = 68;
const int PIECE_REQUEST_SIZE = 17;
const int BLOCK_SIZE = 16 * 1024; // 16 KiB
const int MAX_CONCURRENT_BLOCK_REQUESTS = 4;

/**
 * @brief Represents the connection state of a peer.
 */
struct ConnectionState {
  bool interested = false; ///< Indicates if the peer is interested.
  bool choked = true;      ///< Indicates if the peer is choked.
};

/**
 * @brief Manages the state of a piece being downloaded.
 */
class PieceDownloadState {
public:
  uint32_t piece_index;  ///< Index of the piece.
  uint32_t total_blocks; ///< Total number of blocks in the piece.
  std::vector<bool>
      blocks_received; ///< Tracks which blocks have been received.
  std::vector<std::byte>
      piece_data_buffer;          ///< Buffer to store the entire piece data.
  uint32_t next_block_to_request; ///< Index of the next block to request.

  /**
   * @brief Default constructor for PieceDownloadState.
   */
  PieceDownloadState() // TODO: Create some error handling when this gets
                       // invoked
      : piece_index(0), total_blocks(0), piece_data_buffer(0),
        next_block_to_request(0) {}

  /**
   * @brief Constructs a PieceDownloadState with specified parameters.
   *
   * @param piece_index Index of the piece.
   * @param total_blocks Total number of blocks in the piece.
   * @param piece_size Size of the piece in bytes.
   */
  PieceDownloadState(uint32_t piece_index, uint32_t total_blocks,
                     uint32_t piece_size)
      : piece_index(piece_index), total_blocks(total_blocks),
        blocks_received(total_blocks, false),
        piece_data_buffer(
            piece_size), // Initialize the buffer with the piece size
        next_block_to_request(0) {}
};

/**
 * @brief Manages the connection to a peer in the BitTorrent network.
 */
class PeerConnection : public std::enable_shared_from_this<PeerConnection> {
public:
  /**
   * @brief Constructs a PeerConnection.
   *
   * @param io_context Boost.Asio IO context.
   * @param info_hash Info hash of the torrent.
   * @param peer_id ID of the peer.
   * @param piece_manager Shared pointer to the PieceManager.
   * @param file_manager Shared pointer to the FileManager.
   */
  PeerConnection(boost::asio::io_context &io_context, const InfoHash &info_hash,
                 const Peer::Id &peer_id,
                 std::shared_ptr<PieceManager> piece_manager,
                 std::shared_ptr<LinuxFileManager> file_manager)
      : socket_(io_context), info_hash_(info_hash), peer_id_(peer_id),
        piece_manager_(piece_manager), file_manager_(file_manager) {}

  /**
   * @brief Gets the socket associated with the peer connection.
   *
   * @return Reference to the TCP socket.
   */
  tcp::socket &socket();

  /**
   * @brief Starts the peer connection.
   */
  void start();

  /**
   * @brief Stops the peer connection.
   */
  void stop();

private:
  // These functions really need no explaining, but I will do it anyway so the
  // Doxygen looks a little nicer

  /**
   * @brief Handles the handshake process with the peer.
   *
   * @param error The error code resulting from the handshake attempt.
   */
  void handle_handshake(const boost::system::error_code &error);

  /**
   * @brief Handles the response to the handshake message.
   *
   * @param response The response message from the peer.
   * @param error The error code resulting from reading the handshake response.
   */
  void
  handle_handshake_response(std::shared_ptr<std::vector<std::byte>> response,
                            const boost::system::error_code &error);

  /**
   * @brief Sends an 'interested' message to the peer.
   */
  void send_interested_message();

  /**
   * @brief Handles the response to the 'interested' message.
   *
   * @param error The error code resulting from sending the 'interested'
   * message.
   */
  void handle_interested_message(const boost::system::error_code &error);

  /**
   * @brief Requests a piece from the peer.
   */
  void request_piece();

  /**
   * @brief Sends a block request to the peer.
   *
   * @param piece_index The index of the piece to request.
   * @param block_index The index of the block within the piece to request.
   */
  void send_block_request(uint32_t piece_index, uint32_t block_index);

  /**
   * @brief Requests more blocks for a piece.
   *
   * @param piece_request The state of the piece being downloaded.
   */
  void request_more_blocks(PieceDownloadState &piece_request);

  /**
   * @brief Handles the response to a block request.
   *
   * @param error The error code resulting from the block request.
   */
  void handle_piece_request_response(const boost::system::error_code &error);

  /**
   * @brief Reads a message from the peer.
   */
  void read_message();

  /**
   * @brief Handles reading the length of a message.
   *
   * @param error The error code resulting from reading the message length.
   * @param bytes_transferred The number of bytes transferred during the read.
   */
  void handle_read_length(const boost::system::error_code &error,
                          std::size_t bytes_transferred);

  /**
   * @brief Handles reading the message data.
   *
   * @param error The error code resulting from reading the message.
   * @param bytes_transferred The number of bytes transferred during the read.
   */
  void handle_read_message(const boost::system::error_code &error,
                           std::size_t bytes_transferred);

  /**
   * @brief Processes a received message.
   *
   * @param message The message to process.
   */
  void process_message(const Message &message);

  /**
   * @brief Handles a 'bitfield' message from the peer.
   *
   * @param bitfield The bitfield data.
   */
  void handle_bitfield_message(const std::vector<std::byte> &bitfield);

  /**
   * @brief Handles a 'piece' message from the peer.
   *
   * @param piece_data The piece data received from the peer.
   */
  void handle_piece_message(const PieceData &piece_data);

  tcp::socket socket_;                 ///< TCP socket for the connection.
  InfoHash info_hash_;                 ///< Info hash of the torrent.
  Peer::Id peer_id_;                   ///< ID of the peer.
  std::vector<std::byte> read_buffer_; ///< Buffer for reading messages.
  ConnectionState local_state_;        ///< Local connection state.
  ConnectionState remote_state_;       ///< Remote connection state.
  std::vector<bool> bitfield_; ///< Bitfield of pieces available from the peer.
  std::shared_ptr<PieceManager>
      piece_manager_; ///< Shared pointer to the PieceManager.
  std::shared_ptr<LinuxFileManager>
      file_manager_;             ///< Shared pointer to the FileManager.
  bool request_pending_ = false; ///< Indicates if a request is pending.
  std::unordered_map<uint32_t, PieceDownloadState>
      piece_download_states_; ///< States of pieces being downloaded.
};

#endif // PEERCONNECTION_H
