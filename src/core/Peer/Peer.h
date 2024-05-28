#include <array>
#include <cstdint>
#include <optional>
#include <string>

struct Peer {
  using Id = std::array<std::byte, 20>;

  std::optional<Id> id;
  std::string ip;
  uint16_t port; // Port at which the torrent service is running

  bool is_id_set() const { return id.has_value(); }
};
