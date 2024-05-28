#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "PieceBuffer/PieceBuffer.h"
#include "Torrent/Torrent.h"
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

class FileManager {
public:
  virtual ~FileManager() = default;

  virtual bool write_block(uint32_t piece_index, uint32_t offset,
                           const std::vector<std::byte> &data);

  virtual std::vector<std::byte>
  read_block(uint32_t piece_index, uint32_t offset, uint32_t length) const = 0;

protected:
  virtual void pre_allocate_space() = 0;

  FileManager(const std::vector<FileInfo> &files, uint32_t piece_length,
              std::vector<InfoHash> info_hashes);

  virtual bool write_piece(uint32_t piece_index) = 0;

  uint32_t total_pieces() const;

  std::vector<FileInfo> files_;
  uint32_t piece_length_;

  std::vector<PieceBuffer>
      pieces_; ///< Combined buffers and state tracking for each piece.
};

class LinuxFileManager : public FileManager {
public:
  LinuxFileManager(const std::vector<FileInfo> &files, uint32_t piece_length,
                   std::vector<InfoHash> info_hashes);

  virtual ~LinuxFileManager() override = default;

  virtual std::vector<std::byte> read_block(uint32_t piece_index,
                                            uint32_t offset,
                                            uint32_t length) const override;
  bool write_test_piece(const std::byte *data, uint32_t offset,
                        uint32_t length);

protected:
  virtual void pre_allocate_space() override;
  virtual bool write_piece(uint32_t piece_index) override;

private:
  bool write_to_file(const std::string &path, uint64_t offset,
                     const std::byte *data, uint64_t length);
};

#endif // FILEMANAGER_H
