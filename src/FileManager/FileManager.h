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

// Abstract file manager class
class FileManager {
public:
  virtual ~FileManager() = default;

  virtual bool writeBlock(const size_t pieceIndex, const BlockInfo &block,
                          const std::vector<char> &data) = 0;
  virtual std::vector<char> readBlock(const size_t pieceIndex,
                                      const BlockInfo &block) const = 0;
  virtual void preAllocateSpace() = 0;

  size_t totalFileSize() const;

protected:
  FileManager(const std::vector<FileInfo> &files, const size_t pieceLength);

  // Calculate global block start and end
  std::pair<size_t, size_t> getBlockBounds(const size_t pieceIndex,
                                           const BlockInfo &block) const;
  size_t calculateDataOffset(const BlockInfo &block) const;

  virtual bool writePiece(const size_t pieceIndex) = 0;

  size_t totalPieces() const;
  std::vector<FileInfo> files_;
  size_t pieceLength_;
  std::vector<std::unique_ptr<PieceBuffer>> pieceBuffers_;
};

// File manager for linux systems
class LinuxFileManager : public FileManager {
public:
  LinuxFileManager(const std::vector<FileInfo> &files,
                   const size_t pieceLength_);
  ~LinuxFileManager() override = default;

  bool writeBlock(const size_t pieceIndex, const BlockInfo &block,
                  const std::vector<char> &data) override;

  std::vector<char> readBlock(const size_t pieceIndex,
                              const BlockInfo &block) const override;

  std::vector<char> readPiece(const size_t pieceIndex) const;

private:
  bool writePiece(const size_t pieceIndex) override;
  void preAllocateSpace() override;
};

#endif // FILEMANAGER_H
