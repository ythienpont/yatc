#include "PieceBuffer.h"

PieceBufferInfo::PieceBufferInfo(uint32_t pieceLength, InfoHash infoHash)
    : pieceLength_(pieceLength), infoHash_(infoHash) {}

bool PieceBufferInfo::addBlock(uint32_t offset, uint32_t length) {
  if (blockSize_ == 0) {
    blockSize_ =
        length; // Initialize block size with the length of the first block
    // Calculate the number of blocks required for the whole piece
    uint32_t totalBlocks = (pieceLength_ + blockSize_ - 1) / blockSize_;
    receivedBlocks_.resize(totalBlocks, false);
  }

  uint32_t blockIndex = offset / blockSize_;
  if (blockIndex >= receivedBlocks_.size()) {
    throw std::runtime_error("Block index out of range");
  }

  if (offset % blockSize_ != 0) {
    throw std::runtime_error("Offset does not align with block size");
  }

  receivedBlocks_[blockIndex] = true; // Mark this block as received
  return true;
}

bool PieceBufferInfo::isActive() const {
  for (bool received : receivedBlocks_) {
    if (received)
      return true;
  }
  return false;
}

void PieceBufferInfo::clear() {
  blockSize_ = 0;
  receivedBlocks_.clear();
}

bool PieceBufferInfo::isComplete() const {
  if (receivedBlocks_.empty())
    return false;

  for (bool received : receivedBlocks_) {
    if (!received)
      return false;
  }
  return true;
}
