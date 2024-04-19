#include "PieceBuffer.h"
#include <algorithm>
#include <stdexcept>

bool PieceBuffer::addBlock(const BlockInfo &block,
                           const std::vector<char> &data) {
  if (block.offset + block.length > pieceLength_) {
    return false; // Ensure we do not exceed the piece length limit
  }

  if (data_.size() < block.offset + block.length) {
    data_.resize(block.offset + block.length); // Resize to accommodate new data
  }

  std::copy(data.begin(), data.end(), data_.begin() + block.offset);
  updateReceivedBlocks(block);
  return true;
}

bool PieceBuffer::isComplete() const {
  if (receivedBlocks_.empty())
    return false;
  return receivedBlocks_.front().first == 0 &&
         receivedBlocks_.back().second == pieceLength_;
}

void PieceBuffer::updateReceivedBlocks(const BlockInfo &block) {
  auto newEnd = block.offset + block.length;
  bool merged = false;
  for (auto &existingBlock : receivedBlocks_) {
    if (block.offset <= existingBlock.second && newEnd >= existingBlock.first) {
      existingBlock.first = std::min(existingBlock.first, block.offset);
      existingBlock.second = std::max(existingBlock.second, newEnd);
      merged = true;
      break;
    }
  }
  if (!merged) {
    receivedBlocks_.emplace_back(block.offset, newEnd);
  }
  mergeBlocks();
}

void PieceBuffer::mergeBlocks() {
  if (receivedBlocks_.size() < 2)
    return;

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

const std::vector<char> &PieceBuffer::getData() const {
  if (!isComplete())
    throw std::runtime_error("Attempt to access incomplete data.");
  return data_;
}
