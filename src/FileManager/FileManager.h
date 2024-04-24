#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "PieceBuffer/PieceBuffer.h"
#include "Torrent/Torrent.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @class FileManager
 * @brief Abstract class providing an interface for file read/write operations
 * in a torrent system.
 *
 * Defines the basic functionality for reading and writing pieces and blocks of
 * data associated with torrent files. This class should be subclassed for
 * specific operating systems or file systems to handle file operations
 * appropriately.
 */
class FileManager {
public:
  /**
   * @brief Virtual destructor to allow for cleanup in derived classes.
   */
  virtual ~FileManager() = default;

  /**
   * @brief Writes a block of data to the appropriate file and position.
   *
   * @param pieceIndex Index of the piece to which the block belongs.
   * @param block Details about the block (offset and size).
   * @param data Data to be written.
   * @return true if the write operation was successful, false otherwise.
   */
  virtual bool writeBlock(const size_t pieceIndex, const BlockInfo &block,
                          const std::vector<char> &data) = 0;

  /**
   * @brief Reads a block of data from the appropriate file and position.
   *
   * @param pieceIndex Index of the piece from which to read.
   * @param block Details about the block to read (offset and size).
   * @return Vector containing the read data.
   */
  virtual std::vector<char> readBlock(const size_t pieceIndex,
                                      const BlockInfo &block) const = 0;

  /**
   * @brief Pre-allocates disk space to optimize file writing operations.
   */
  virtual void preAllocateSpace() = 0;

  /**
   * @brief Calculates the total size of all files managed by the FileManager.
   *
   * @return Size of all files in bytes.
   */
  size_t totalFileSize() const;

protected:
  /**
   * @brief Constructor initializing the FileManager with a list of files and
   * piece length.
   *
   * @param files Vector of FileInfo structures detailing the files.
   * @param pieceLength Length of each piece in bytes.
   */
  FileManager(const std::vector<FileInfo> &files, const size_t pieceLength);

  /**
   * @brief Calculates the global start and end offsets for a given block within
   * a piece.
   *
   * @param pieceIndex Index of the piece.
   * @param block BlockInfo structure describing the block.
   * @return Pair of size_t values indicating the start and end offsets.
   */
  std::pair<size_t, size_t> getBlockBounds(const size_t pieceIndex,
                                           const BlockInfo &block) const;

  /**
   * @brief Calculates the offset of data within a block.
   *
   * @param block Details about the block.
   * @return Offset of the data within the block.
   */
  size_t calculateDataOffset(const BlockInfo &block) const;

  /**
   * @brief Writes a full piece to the appropriate location.
   *
   * @param pieceIndex Index of the piece to write.
   * @return true if successful, false otherwise.
   */
  virtual bool writePiece(const size_t pieceIndex) = 0;

  /**
   * @brief Returns the total number of pieces based on the file sizes and piece
   * length.
   *
   * @return Total number of pieces.
   */
  size_t totalPieces() const;

  std::vector<FileInfo> files_; ///< List of files associated with the torrent.
  size_t pieceLength_;          ///< Length of each piece in bytes.
  std::vector<std::unique_ptr<PieceBuffer>>
      pieceBuffers_; ///< Buffers for managing piece data.
};

/**
 * @class LinuxFileManager
 * @brief FileManager implementation for Linux systems.
 *
 * Handles file operations specifically optimized for Linux environments,
 * utilizing system calls for efficient file reading and writing.
 */
class LinuxFileManager : public FileManager {
public:
  /**
   * @brief Constructs a LinuxFileManager with specified files and piece length.
   *
   * @param files Vector of FileInfo structures detailing the files.
   * @param pieceLength Length of each piece in bytes.
   */
  LinuxFileManager(const std::vector<FileInfo> &files,
                   const size_t pieceLength_);

  /**
   * @brief Default destructor.
   */
  ~LinuxFileManager() override = default;

  bool writeBlock(const size_t pieceIndex, const BlockInfo &block,
                  const std::vector<char> &data) override;

  std::vector<char> readBlock(const size_t pieceIndex,
                              const BlockInfo &block) const override;

  /**
   * @brief Reads a full piece from the file system.
   *
   * @param pieceIndex Index of the piece to read.
   * @return Vector containing the piece data.
   */
  std::vector<char> readPiece(const size_t pieceIndex) const;

private:
  bool writePiece(const size_t pieceIndex) override;
  void preAllocateSpace() override;
};

#endif // FILEMANAGER_H
