#include "PieceBuffer.h"
#include <algorithm>

PieceBufferInfo::PieceBufferInfo(uint32_t pieceLength, uint32_t blockSize)
    : pieceLength_(pieceLength), blockSize_(blockSize),
      lastBlockSize_((pieceLength % blockSize) ? (pieceLength % blockSize)
                                               : blockSize) {}

bool PieceBufferInfo::addBlock(uint32_t offset, uint32_t length) {
  uint32_t newEnd = offset + length;
  if (newEnd > pieceLength_) {
    return false;
  }

  bool merged = false;
  for (auto &block : receivedBlocks_) {
    if (offset <= block.second && newEnd >= block.first) {
      block.first = std::min(block.first, offset);
      block.second = std::max(block.second, newEnd);
      merged = true;
      break;
    }
  }
  if (!merged) {
    receivedBlocks_.emplace_back(offset, newEnd);
  }
  mergeBlocks();
  return true;
}

bool PieceBufferInfo::isComplete() const {
  return (!receivedBlocks_.empty() && receivedBlocks_.front().first == 0 &&
          receivedBlocks_.back().second == pieceLength_);
}

std::vector<uint32_t> PieceBufferInfo::getMissingBlockIndices() const {
  std::vector<uint32_t> missingIndices;
  uint32_t current = 0;
  for (const auto &block : receivedBlocks_) {
    uint32_t blockStartIndex = block.first / blockSize_;
    uint32_t blockEndIndex = (block.second - 1) / blockSize_;
    for (uint32_t idx = current; idx < blockStartIndex; ++idx) {
      missingIndices.push_back(idx);
    }
    current = blockEndIndex + 1;
  }

  uint32_t totalBlocks = (pieceLength_ + blockSize_ - 1) / blockSize_;
  for (uint32_t idx = current; idx < totalBlocks; ++idx) {
    missingIndices.push_back(idx);
  }

  return missingIndices;
}

void PieceBufferInfo::mergeBlocks() {
  std::sort(receivedBlocks_.begin(), receivedBlocks_.end());
  size_t i = 0;
  for (size_t j = 1; j < receivedBlocks_.size(); j++) {
    if (receivedBlocks_[i].second >= receivedBlocks_[j].first) {
      receivedBlocks_[i].second =
          std::max(receivedBlocks_[i].second, receivedBlocks_[j].second);
    } else {
      i++;
      receivedBlocks_[i] = receivedBlocks_[j];
    }
  }
  receivedBlocks_.resize(i + 1);
}
