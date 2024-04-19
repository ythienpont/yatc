#include <cstddef>
#include <vector>

struct BlockInfo {
  size_t offset; // Offset within the piece
  size_t length; // Length of the block
  BlockInfo(size_t _offset, size_t _length)
      : offset(_offset), length(_length) {}

  // Used for sorting
  bool operator<(const BlockInfo &other) const {
    return this->offset < other.offset;
  }
};

class PieceBuffer {
public:
  explicit PieceBuffer(size_t pieceLength) : pieceLength_(pieceLength) {}

  // Adds a block of data to the piece at a specified offset.
  bool addBlock(const BlockInfo &block, const std::vector<char> &data);

  // Checks if the entire piece has been received.
  bool isComplete() const;

  /*
   * Retrieves the data of the piece.
   *
   * This method returns a reference to the internal data buffer. It should only
   * be called if the piece is complete. If called on an incomplete piece, it
   * throws a runtime_error.
   */
  const std::vector<char> &getData() const;

private:
  size_t pieceLength_;
  std::vector<char> data_;

  // List of ranges representing received blocks; each pair holds the start
  // offset and the exclusive end offset of a block.
  std::vector<std::pair<size_t, size_t>> receivedBlocks_;

  //
  void updateReceivedBlocks(const BlockInfo &block);

  // Merges contiguous and overlapping blocks in receivedBlocks_.
  void mergeBlocks();
};
