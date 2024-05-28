#ifndef PIECEBUFFER_H
#define PIECEBUFFER_H

#include "Torrent/Torrent.h"
#include <cstdint>
#include <vector>

class PieceBufferInfo {
public:
  PieceBufferInfo(uint32_t piece_length, InfoHash info_hash);

  bool add_block(uint32_t offset, uint32_t length);
  bool is_complete() const;
  bool is_active() const;
  void clear();
  std::vector<uint32_t> get_missing_block_indices() const;

  uint32_t piece_length_ = 0;

private:
  uint32_t block_size_ = 0;
  InfoHash info_hash_;

  std::vector<bool> received_blocks_;
};

class PieceBufferData {
public:
  explicit PieceBufferData(uint32_t piece_length) : data_(piece_length) {}

  void add_data(uint32_t offset, const std::vector<std::byte> &data);

  const std::vector<std::byte> &get_data() const { return data_; }

private:
  std::vector<std::byte> data_;
};

class PieceBuffer {
public:
  PieceBuffer(uint32_t piece_length, InfoHash info_hash)
      : buffer_(nullptr),
        info_(std::make_unique<PieceBufferInfo>(piece_length, info_hash)) {}

  bool write_block(uint32_t offset, const std::vector<std::byte> &data);

  std::unique_ptr<PieceBufferData> buffer_;
  std::unique_ptr<PieceBufferInfo> info_;
};

#endif // PIECEBUFFER_H
