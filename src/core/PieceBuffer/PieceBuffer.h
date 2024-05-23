#ifndef PIECEBUFFER_H
#define PIECEBUFFER_H

#include "Torrent/Torrent.h"
#include <cstdint>
#include <stdexcept>
#include <vector>

class PieceBufferInfo {
public:
  PieceBufferInfo(uint32_t pieceLength, InfoHash infoHash);

  bool addBlock(uint32_t offset, uint32_t length);
  bool isComplete() const;
  bool isActive() const;
  void clear();
  std::vector<uint32_t> getMissingBlockIndices() const;

private:
  uint32_t pieceLength_ = 0;
  uint32_t blockSize_ = 0;
  InfoHash infoHash_;

  std::vector<bool> receivedBlocks_;
};

struct PieceBuffer {
  explicit PieceBuffer(uint32_t pieceLength) : data_(pieceLength) {}

  void addData(uint32_t offset, const std::vector<char> &data) {
    if (offset + data.size() > data_.size()) {
      throw std::runtime_error("Data overflow attempt");
    }
    std::copy(data.begin(), data.end(), data_.begin() + offset);
  }

  const std::vector<char> &getData() const { return data_; }

  std::vector<char> data_;
};

#endif // PIECEBUFFER_H
