#include <array>
#include <cstdint>
#include <optional>
#include <string>

/**
 * @brief Represents a peer in the BitTorrent network.
 */
struct Peer {
  /**
   * @brief Type alias for a peer ID, represented as a 20-byte array.
   */
  using Id = std::array<std::byte, 20>;

  /**
   * @brief The ID of the peer.
   * 
   * This is an optional field. If the peer ID is known, it will be set.
   */
  std::optional<Id> id;

  /**
   * @brief The IP address of the peer.
   * 
   * This is a string representation of the IP address (e.g., "192.168.1.1").
   */
  std::string ip;

  /**
   * @brief The port number at which the torrent service is running.
   */
  uint16_t port;

  /**
   * @brief Checks if the peer ID is set.
   * 
   * @return true if the peer ID is set, false otherwise.
   */
  bool is_id_set() const { return id.has_value(); }
};
