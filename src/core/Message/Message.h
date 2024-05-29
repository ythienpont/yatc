#ifndef MESSAGE_H
#define MESSAGE_H

#include "Utils/utils.h"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <variant>
#include <vector>

/**
 * @brief Helper struct to allow visitation of multiple variant types.
 * 
 * This utility helps in applying a visitor to a std::variant with multiple types.
 */
template <typename... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

// Explicit deduction guide (not needed in C++20)
template <typename... Ts> overloaded(Ts...) -> overloaded<Ts...>;

/**
 * @brief Enum representing different types of BitTorrent messages.
 */
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

/**
 * @brief Struct representing the data for a Piece message.
 */
struct PieceData {
  uint32_t index;               ///< Index of the piece.
  uint32_t begin;               ///< Beginning offset within the piece.
  std::vector<std::byte> block; ///< Block of data within the piece.
};

/**
 * @brief Alias for the payload of a BitTorrent message.
 */
using Payload = std::variant<
    std::monostate,                  ///< For messages with no payload.
    uint32_t,                        ///< For Have message (piece index).
    std::vector<std::byte>,          ///< For Bitfield and Piece messages.
    std::tuple<uint32_t, uint32_t, uint32_t>, ///< For Request and Cancel messages (index, begin, length).
    PieceData>;                      ///< For Piece message (index, begin, block).

/**
 * @brief Struct representing a BitTorrent message.
 */
struct Message {
  MessageType type; ///< The type of the message.
  Payload payload;  ///< The payload of the message.

  /**
   * @brief Constructs a Message with the specified type and payload.
   * 
   * @param type The type of the message.
   * @param payload The payload of the message.
   */
  Message(MessageType type, Payload payload)
      : type(type), payload(std::move(payload)) {}

  /**
   * @brief Parses a vector of bytes into a Message.
   * 
   * @param data The byte vector containing the message data.
   * @return The parsed Message.
   * 
   * @throws std::runtime_error if the message data is invalid.
   */
  static Message parseMessage(const std::vector<std::byte> &data) {
    if (data.size() < 5)
      throw std::runtime_error("Invalid message data received.");

    MessageType type = static_cast<MessageType>(static_cast<uint8_t>(data[4]));
    std::vector<std::byte> payloadBytes(data.begin() + 5, data.end());

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
        throw std::runtime_error("Invalid payload size for Request/Cancel message.");
      return {type, std::make_tuple(bytes_to_uint32(payloadBytes, 0),
                                    bytes_to_uint32(payloadBytes, 4),
                                    bytes_to_uint32(payloadBytes, 8))};

    case MessageType::Piece:
      if (payloadBytes.size() < 2 * sizeof(uint32_t))
        throw std::runtime_error("Invalid payload size for Piece message.");
      {
        uint32_t index = bytes_to_uint32(payloadBytes, 0);
        uint32_t begin = bytes_to_uint32(payloadBytes, 4);
        std::vector<std::byte> block(payloadBytes.begin() + 8, payloadBytes.end());
        return {type, PieceData{index, begin, std::move(block)}};
      }

    default:
      throw std::runtime_error("Unknown message type.");
    }
  }
};

#endif // MESSAGE_H
