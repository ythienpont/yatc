#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "PieceBuffer/PieceBuffer.h"
#include "Torrent/Torrent.h"
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

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
   * @param offset Offset in bytes within the piece.
   * @param data Data to be written.
   * @return true if the write operation was successful, false otherwise.
   */
  virtual bool writeBlock(uint32_t pieceIndex, uint32_t offset,
                          const std::vector<char> &data) = 0;

  /**
   * @brief Reads a block of data from the appropriate file and position.
   *
   * @param pieceIndex Index of the piece from which to read.
   * @param offset Offset in bytes within the piece.
   * @param length Length of the data to read in bytes.
   * @return Vector containing the read data.
   */
  virtual std::vector<char> readBlock(uint32_t pieceIndex, uint32_t offset,
                                      uint32_t length) const = 0;

  /**
   * @brief Pre-allocates disk space to optimize file writing operations.
   */
  virtual void preAllocateSpace() = 0;

protected:
  /**
   * @brief Constructor initializing the FileManager with a list of files and
   * piece length.
   *
   * @param files Vector of FileInfo structures detailing the files.
   * @param pieceLength Length of each piece in bytes.
   */
  FileManager(const std::vector<FileInfo> &files, uint32_t pieceLength,
              std::vector<InfoHash> infoHashes);

  /**
   * @brief Writes a full piece to the appropriate location.
   *
   * @param pieceIndex Index of the piece to write.
   * @return true if successful, false otherwise.
   */
  virtual bool writePiece(uint32_t pieceIndex) = 0;

  /**
   * @brief Returns the total number of pieces based on the file sizes and piece
   * length.
   *
   * @return Total number of pieces.
   */
  size_t totalPieces() const;

  std::vector<FileInfo> files_; ///< List of files associated with the torrent.
  uint32_t pieceLength_;        ///< Length of each piece in bytes.

  struct PieceData {
    std::unique_ptr<PieceBuffer> buffer;
    std::unique_ptr<PieceBufferInfo> info;
  };

  std::vector<PieceData>
      pieces_; ///< Combined buffers and state tracking for each piece.
};

class LinuxFileManager : public FileManager {
public:
  LinuxFileManager(const std::vector<FileInfo> &files, uint32_t pieceLength,
                   std::vector<InfoHash> infoHashes);

  virtual ~LinuxFileManager() override = default;

  virtual bool writeBlock(uint32_t pieceIndex, uint32_t offset,
                          const std::vector<char> &data) override;

  virtual std::vector<char> readBlock(uint32_t pieceIndex, uint32_t offset,
                                      uint32_t length) const override;

  virtual void preAllocateSpace() override;

protected:
  virtual bool writePiece(uint32_t pieceIndex) override;

private:
  bool writeToFile(const std::string &path, uint64_t offset, const char *data,
                   uint64_t length);
};

#endif // FILEMANAGER_H
