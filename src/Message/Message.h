#ifndef MESSAGE_H
#define MESSAGE_H

#include "Utils/utils.h"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <variant>
#include <vector>

template <typename... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

// Explicit deduction guide (not needed in C++20)
template <typename... Ts> overloaded(Ts...) -> overloaded<Ts...>;

enum class MessageType : uint8_t {
  Choke = 0,
  Unchoke = 1,
  Interested = 2,
  NotInterested = 3,
  Have = 4,
  Bitfield = 5,
  Request = 6,
  Piece = 7,
  Cancel = 8,
};

using Payload =
    std::variant<std::monostate,         // for messages with no payload
                 uint32_t,               // for Have message (piece index)
                 std::vector<std::byte>, // for Bitfield and Piece
                 std::tuple<uint32_t, uint32_t, uint32_t>>; // for Request and
                                                            // Cancel (index,
                                                            // begin, length)

struct Message {
  MessageType type;
  Payload payload;

  Message(MessageType type, Payload payload)
      : type(type), payload(std::move(payload)) {}

  static Message parseMessage(const std::vector<std::byte> &data) {
    if (data.empty())
      throw std::runtime_error("Empty data vector received.");

    MessageType type = static_cast<MessageType>(static_cast<uint8_t>(data[0]));
    std::vector<std::byte> payloadBytes(data.begin() + 1, data.end());

    switch (type) {
    case MessageType::Choke:
    case MessageType::Unchoke:
    case MessageType::Interested:
    case MessageType::NotInterested:
      return {type, std::monostate{}};

    case MessageType::Have:
      if (payloadBytes.size() < sizeof(uint32_t))
        throw std::runtime_error("Invalid payload size for Have message.");
      return {type, bytes_to_uint32(payloadBytes)};

    case MessageType::Bitfield:
      return {type, payloadBytes};

    case MessageType::Request:
    case MessageType::Cancel:
      if (payloadBytes.size() < 3 * sizeof(uint32_t))
        throw std::runtime_error(
            "Invalid payload size for Request/Cancel message.");
      return {type, std::make_tuple(bytes_to_uint32(payloadBytes, 0),
                                    bytes_to_uint32(payloadBytes, 4),
                                    bytes_to_uint32(payloadBytes, 8))};

    default:
      throw std::runtime_error("Unknown message type.");
    }
  }
};

#endif //! MESSAGE_H
