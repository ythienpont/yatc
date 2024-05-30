#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "Torrent/Torrent.h"
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

/**
 * @brief Abstract base class for managing torrent files.
 *
 * This class provides the interface for reading and writing pieces of a
 * torrent.
 */
class FileManager {
public:
  /**
   * @brief Virtual destructor.
   */
  virtual ~FileManager() = default;

  /**
   * @brief Reads a block of data from a piece.
   *
   * @param piece_index The index of the piece to read from.
   * @param offset The offset within the piece to start reading.
   * @param length The number of bytes to read.
   * @return A vector of bytes containing the data read.
   */
  virtual std::vector<std::byte>
  read_block(uint32_t piece_index, uint32_t offset, uint32_t length) const = 0;

  /**
   * @brief Writes a piece of data.
   *
   * @param piece_index The index of the piece to write.
   * @param data The data to write.
   * @return true if the piece was written successfully, false otherwise.
   */
  virtual bool write_piece(uint32_t piece_index,
                           std::vector<std::byte> &data) = 0;

protected:
  /**
   * @brief Pre-allocates space for the files.
   */
  virtual void pre_allocate_space() = 0;

  /**
   * @brief Constructs a FileManager.
   *
   * @param files The list of files in the torrent.
   * @param piece_length The length of each piece in bytes.
   * @param info_hashes The info hashes of the torrent.
   */
  FileManager(const std::vector<FileInfo> &files, uint32_t piece_length,
              std::vector<InfoHash> info_hashes);

  /**
   * @brief Gets the total number of pieces.
   *
   * @return The total number of pieces.
   */
  uint32_t total_pieces() const;

  std::vector<FileInfo> files_; ///< The list of files in the torrent.
  uint32_t piece_length_;       ///< The length of each piece in bytes.

  mutable std::mutex
      file_mutex_; ///< Mutex for thread-safe access to file operations.
};

/**
 * @brief Manages torrent files on a Linux system.
 *
 * This class provides Linux-specific implementations for reading and writing
 * pieces of a torrent.
 */
class LinuxFileManager : public FileManager {
public:
  /**
   * @brief Constructs a LinuxFileManager.
   *
   * @param files The list of files in the torrent.
   * @param piece_length The length of each piece in bytes.
   * @param info_hashes The info hashes of the torrent.
   */
  LinuxFileManager(const std::vector<FileInfo> &files, uint32_t piece_length,
                   std::vector<InfoHash> info_hashes);

  /**
   * @brief Virtual destructor.
   */
  virtual ~LinuxFileManager() override = default;

  /**
   * @brief Reads a block of data from a piece.
   *
   * @param piece_index The index of the piece to read from.
   * @param offset The offset within the piece to start reading.
   * @param length The number of bytes to read.
   * @return A vector of bytes containing the data read.
   */
  virtual std::vector<std::byte> read_block(uint32_t piece_index,
                                            uint32_t offset,
                                            uint32_t length) const override;

  /**
   * @brief Writes a piece of data.
   *
   * @param piece_index The index of the piece to write.
   * @param data The data to write.
   * @return true if the piece was written successfully, false otherwise.
   */
  virtual bool write_piece(uint32_t piece_index,
                           std::vector<std::byte> &data) override;

protected:
  /**
   * @brief Pre-allocates space for the files.
   */
  virtual void pre_allocate_space() override;

private:
  /**
   * @brief Writes data to a file.
   *
   * @param path The path of the file.
   * @param offset The offset within the file to start writing.
   * @param data The data to write.
   * @param length The number of bytes to write.
   * @return true if the data was written successfully, false otherwise.
   */
  bool write_to_file(const std::string &path, uint64_t offset,
                     const std::byte *data, uint64_t length);
};

#endif // FILEMANAGER_H
