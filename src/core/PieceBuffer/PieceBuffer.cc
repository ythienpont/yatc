#include "PieceBuffer.h"
#include <stdexcept>

PieceBufferInfo::PieceBufferInfo(uint32_t piece_length, InfoHash info_hash)
    : piece_length_(piece_length), info_hash_(info_hash) {}

bool PieceBufferInfo::add_block(uint32_t offset, uint32_t length) {
  if (block_size_ == 0) {
    block_size_ =
        length; // Initialize block size with the length of the first block
    // Calculate the number of blocks required for the whole piece
    uint32_t total_blocks = (piece_length_ + block_size_ - 1) / block_size_;
    received_blocks_.resize(total_blocks, false);
  }

  uint32_t block_index = offset / block_size_;
  if (block_index >= received_blocks_.size()) {
    throw std::runtime_error("Block index out of range");
  }

  if (offset % block_size_ != 0) {
    throw std::runtime_error("Offset does not align with block size");
  }

  received_blocks_[block_index] = true; // Mark this block as received
  return true;
}

bool PieceBufferInfo::is_active() const {
  for (bool received : received_blocks_) {
    if (received)
      return true;
  }
  return false;
}

void PieceBufferInfo::clear() {
  block_size_ = 0;
  received_blocks_.clear();
}

bool PieceBufferInfo::is_complete() const {
  if (received_blocks_.empty())
    return false;

  for (bool received : received_blocks_) {
    if (!received)
      return false;
  }
  return true;
}

std::vector<uint32_t> PieceBufferInfo::get_missing_block_indices() const {
  std::vector<uint32_t> missing_blocks;
  for (uint32_t i = 0; i < received_blocks_.size(); ++i) {
    if (!received_blocks_[i]) {
      missing_blocks.push_back(i);
    }
  }
  return missing_blocks;
}
void PieceBufferData::add_data(uint32_t offset,
                               const std::vector<std::byte> &data) {
  if (offset + data.size() > data_.size()) {
    throw std::runtime_error("Data overflow attempt");
  }
  std::copy(data.begin(), data.end(), data_.begin() + offset);
}

bool PieceBuffer::write_block(uint32_t offset,
                              const std::vector<std::byte> &data) {
  if (!info_->add_block(offset, data.size())) {
    return false;
  }

  if (!buffer_) {
    buffer_ = std::make_unique<PieceBufferData>(info_->piece_length_);
  }

  buffer_->add_data(offset, data);
  return true;
}
