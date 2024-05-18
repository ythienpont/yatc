#ifndef PIECEBUFFER_H
#define PIECEBUFFER_H

#include <cstdint>
#include <vector>

class PieceBufferInfo {
public:
  PieceBufferInfo(uint32_t pieceLength, uint32_t blockSize);

  bool addBlock(uint32_t offset, uint32_t length);
  bool isComplete() const;
  std::vector<uint32_t> getMissingBlockIndices() const;

private:
  uint32_t pieceLength_;
  uint32_t blockSize_;
  uint32_t lastBlockSize_;
  std::vector<std::pair<uint32_t, uint32_t>> receivedBlocks_;

  void mergeBlocks();
};

class PieceBuffer {
public:
  explicit PieceBuffer(uint32_t pieceLength) : data_(pieceLength) {}

  void addData(uint32_t offset, const std::vector<char> &data) {
    std::copy(data.begin(), data.end(), data_.begin() + offset);
  }

  const std::vector<char> &getData() const { return data_; }

private:
  std::vector<char> data_;
};

#endif // PIECEBUFFER_H
