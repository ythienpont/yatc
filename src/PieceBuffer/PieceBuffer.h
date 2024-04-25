#ifndef PIECEBUFFER_H
#define PIECEBUFFER_H

#include <cstddef>
#include <vector>

/**
 * @struct BlockInfo
 * @brief Stores information about a block within a torrent piece.
 *
 * Contains the offset within the piece and the length of the block. This
 * structure also provides comparison operators for sorting based on the offset.
 */
struct BlockInfo {
  size_t offset; ///< Offset of the block within the piece.
  size_t length; ///< Length of the block.

  /**
   * @brief Construct a new Block Info object.
   *
   * @param _offset Offset of the block within the piece.
   * @param _length Length of the block.
   */
  BlockInfo(size_t _offset, size_t _length)
      : offset(_offset), length(_length) {}

  /**
   * @brief Compares this block's offset with another block's offset for
   * ordering.
   *
   * @param other The other BlockInfo to compare against.
   * @return true if this block's offset is less than the other's, false
   * otherwise.
   */
  bool operator<(const BlockInfo &other) const {
    return this->offset < other.offset;
  }
};

/**
 * @class PieceBuffer
 * @brief Manages the data of a single piece of a torrent, handling block
 * addition and completeness checking.
 *
 * This class is responsible for storing block data in the correct sequence
 * within a piece and determining when all blocks of a piece have been received.
 */
class PieceBuffer {
public:
  /**
   * @brief Constructs a PieceBuffer object with a specified piece length.
   *
   * @param pieceLength Length of the piece in bytes.
   */
  PieceBuffer(size_t pieceLength);

  /**
   * @brief Adds a block of data to the piece at the specified offset.
   *
   * @param block Information about the block including its offset and length.
   * @param data The actual data of the block.
   * @return true if the block was successfully added, false otherwise.
   */
  bool addBlock(const BlockInfo &block, const std::vector<char> &data);

  /**
   * @brief Checks if the entire piece has been received.
   *
   * @return true if all blocks have been received and the piece is complete,
   * false otherwise.
   */
  bool isComplete() const;

  /**
   * @brief Retrieves the data of the piece.
   *
   * This method returns a reference to the internal data buffer. It should only
   * be called if the piece is complete. If called on an incomplete piece, it
   * throws a runtime_error.
   *
   * @return const reference to the vector containing the piece's data.
   * @throws runtime_error if the piece is not complete.
   */
  const std::vector<char> &getData() const;

private:
  size_t pieceLength_;     ///< The length of the piece in bytes.
  std::vector<char> data_; ///< Buffer containing the piece's data.

  /// List of ranges representing received blocks; each pair holds the start
  /// offset and the exclusive end offset of a block.
  std::vector<std::pair<size_t, size_t>> receivedBlocks_;

  /**
   * @brief Updates the list of received blocks when a new block is added.
   *
   * @param block Information about the new block to add.
   */
  void updateReceivedBlocks(const BlockInfo &block);

  /**
   * @brief Merges contiguous and overlapping blocks in receivedBlocks_.
   *
   * Optimizes the list of received blocks by merging adjacent or overlapping
   * blocks into single ranges. This helps in efficiently checking for
   * completeness.
   */
  void mergeBlocks();
};

#endif // PIECEBUFFER_H
