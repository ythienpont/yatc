
#include "Message/Message.h"
#include <gtest/gtest.h>
#include <tuple>
#include <vector>

TEST(MessageTest, ConstructionChoke) {
  Message msg(MessageType::Choke, std::monostate{});
  EXPECT_EQ(msg.type, MessageType::Choke);
  EXPECT_TRUE(std::holds_alternative<std::monostate>(msg.payload));
}

TEST(MessageTest, ConstructionHave) {
  uint32_t index = 5;
  Message msg(MessageType::Have, index);
  EXPECT_EQ(msg.type, MessageType::Have);
  EXPECT_TRUE(std::holds_alternative<uint32_t>(msg.payload));
  EXPECT_EQ(std::get<uint32_t>(msg.payload), index);
}

TEST(MessageTest, ConstructionBitfield) {
  std::vector<std::byte> bitfield = {std::byte{0xFF}, std::byte{0x0F}};
  Message msg(MessageType::Bitfield, bitfield);
  EXPECT_EQ(msg.type, MessageType::Bitfield);
  EXPECT_TRUE(std::holds_alternative<std::vector<std::byte>>(msg.payload));
  EXPECT_EQ(std::get<std::vector<std::byte>>(msg.payload), bitfield);
}

/*
TEST(MessageTest, ConstructionRequest) {
  std::tuple<uint32_t, uint32_t, uint32_t> request = {1, 2, 3};
  Message msg(MessageType::Request, request);
  EXPECT_EQ(msg.type, MessageType::Request);
  EXPECT_TRUE(std::holds_alternative<std::tuple<uint32_t, uint32_t,
uint32_t>>(msg.payload)); EXPECT_EQ(std::get<std::tuple<uint32_t, uint32_t,
uint32_t>>(msg.payload)(request));
}
*/

TEST(MessageTest, ConstructionPiece) {
  PieceData piece{1, 0, {std::byte{0xAB}, std::byte{0xCD}}};
  Message msg(MessageType::Piece, piece);
  EXPECT_EQ(msg.type, MessageType::Piece);
  EXPECT_TRUE(std::holds_alternative<PieceData>(msg.payload));
  PieceData payload = std::get<PieceData>(msg.payload);
  EXPECT_EQ(payload.index, piece.index);
  EXPECT_EQ(payload.begin, piece.begin);
  EXPECT_EQ(payload.block, piece.block);
}

TEST(MessageTest, ParseChokeMessage) {
  std::vector<std::byte> data = {std::byte{0x00}, std::byte{0x00},
                                 std::byte{0x00}, std::byte{0x01},
                                 std::byte{0x00}};
  Message msg = Message::parseMessage(data);
  EXPECT_EQ(msg.type, MessageType::Choke);
  EXPECT_TRUE(std::holds_alternative<std::monostate>(msg.payload));
}

TEST(MessageTest, ParseHaveMessage) {
  std::vector<std::byte> data = {
      std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
      std::byte{0x05}, std::byte{0x04}, std::byte{0x00},
      std::byte{0x00}, std::byte{0x00}, std::byte{0x05}};
  Message msg = Message::parseMessage(data);
  EXPECT_EQ(msg.type, MessageType::Have);
  EXPECT_TRUE(std::holds_alternative<uint32_t>(msg.payload));
  EXPECT_EQ(std::get<uint32_t>(msg.payload), 5);
}

TEST(MessageTest, ParseBitfieldMessage) {
  std::vector<std::byte> data = {
      std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x02},
      std::byte{0x05}, std::byte{0xFF}, std::byte{0x0F}};
  Message msg = Message::parseMessage(data);
  EXPECT_EQ(msg.type, MessageType::Bitfield);
  EXPECT_TRUE(std::holds_alternative<std::vector<std::byte>>(msg.payload));
  EXPECT_EQ(std::get<std::vector<std::byte>>(msg.payload),
            (std::vector<std::byte>{std::byte{0xFF}, std::byte{0x0F}}));
}

/*
TEST(MessageTest, ParseRequestMessage) {
  std::vector<std::byte> data = {
      std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x0D},
      std::byte{0x06}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
      std::byte{0x01}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
      std::byte{0x02}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
      std::byte{0x03}};
  Message msg = Message::parseMessage(data);
  EXPECT_EQ(msg.type, MessageType::Request);
  EXPECT_TRUE(std::holds_alternative<std::tuple<uint32_t, uint32_t, uint32_t>>(
      msg.payload));
  EXPECT_EQ(std::get<std::tuple<uint32_t, uint32_t, uint32_t>>(msg.payload),
            std::make_tuple(1, 2, 3));
}
*/

TEST(MessageTest, ParseInvalidMessageTooShort) {
  std::vector<std::byte> data = {std::byte{0x00}, std::byte{0x00},
                                 std::byte{0x00}, std::byte{0x01}};
  EXPECT_THROW(Message::parseMessage(data), std::runtime_error);
}

TEST(MessageTest, ParseInvalidMessageUnknownType) {
  std::vector<std::byte> data = {std::byte{0x00}, std::byte{0x00},
                                 std::byte{0x00}, std::byte{0x01},
                                 std::byte{0xFF}};
  EXPECT_THROW(Message::parseMessage(data), std::runtime_error);
}
