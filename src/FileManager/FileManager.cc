#include "FileManager.h"
#include "Torrent/Torrent.h"
#include <fstream>
#include <memory>

size_t FileManager::totalFileSize() const {
  size_t totalFileSize = 0;

  for (const auto &file : files_) {
    totalFileSize += file.length;
  }

  return totalFileSize;
}

FileManager::FileManager(const std::vector<FileInfo> &files,
                         const size_t pieceLength)
    : files_(files), pieceLength_(pieceLength) {
  size_t numPieces = totalPieces();
  pieceBuffers_.resize(numPieces);

  for (size_t i = 0; i < numPieces; i++) {
    pieceBuffers_[i] = nullptr;
  }
}

size_t FileManager::totalPieces() const {
  return (totalFileSize() + pieceLength_ - 1) / pieceLength_;
}

std::pair<size_t, size_t>
FileManager::getBlockBounds(const size_t pieceIndex,
                            const BlockInfo &block) const {
  size_t start = pieceIndex * pieceLength_ + block.offset;
  size_t end = start + block.length;

  return {start, end};
}

LinuxFileManager::LinuxFileManager(const std::vector<FileInfo> &files,
                                   const size_t pieceLength)
    : FileManager(files, pieceLength) {
  preAllocateSpace();
}

bool LinuxFileManager::writeBlock(const size_t pieceIndex,
                                  const BlockInfo &block,
                                  const std::vector<char> &data) {
  // TODO: Check if have in piecemanager
  if (pieceIndex >= pieceBuffers_.size())
    throw std::out_of_range("Piece index is out of range.");

  if (pieceBuffers_[pieceIndex] == nullptr) {
    pieceBuffers_[pieceIndex] = std::make_unique<PieceBuffer>(pieceLength_);
  }
  if (pieceBuffers_[pieceIndex]->addBlock(block, data))
    return false;

  if (pieceBuffers_[pieceIndex]->isComplete() && writePiece(pieceIndex)) {
    // TODO: Check hash before writing piece
    pieceBuffers_[pieceIndex].reset(); // Reset buffer after writing to disk
    // TODO: Set to have in piecemanager
  }

  return true;
}

bool LinuxFileManager::writePiece(const size_t pieceIndex) {
  // TODO: Check if have in piecemanager
  if (pieceIndex >= pieceBuffers_.size() || !pieceBuffers_[pieceIndex])
    throw std::invalid_argument("Invalid piece index or uninitialized buffer.");

  const auto &pieceData = pieceBuffers_[pieceIndex]->getData();
  auto [pieceStart, pieceEnd] = getBlockBounds(pieceIndex, {0, pieceLength_});

  for (const auto &file : files_) {
    if (pieceEnd <= file.startOffset || pieceStart >= file.endOffset)
      continue; // Piece does not intersect with this file

    size_t fileWriteStart =
        std::max(pieceStart, file.startOffset) - file.startOffset;
    size_t dataOffset = std::max(file.startOffset, pieceStart) - pieceStart;
    size_t writeSize = std::min(file.endOffset, pieceEnd) -
                       std::max(file.startOffset, pieceStart);

    std::fstream fileStream(file.path,
                            std::ios::in | std::ios::out | std::ios::binary);
    if (!fileStream)
      throw std::runtime_error("Failed to open file: " + file.path);

    fileStream.seekp(fileWriteStart);
    fileStream.write(&pieceData[dataOffset], writeSize);
    if (!fileStream.good())
      throw std::runtime_error("Failed to write data to file: " + file.path);
  }

  return true;
}

std::vector<char> LinuxFileManager::readBlock(const size_t pieceIndex,
                                              const BlockInfo &block) const {
  if (pieceIndex >= totalPieces())
    throw std::out_of_range("Piece index is out of range.");

  // Calculate the global offset in the torrent data
  auto [blockStart, blockEnd] = getBlockBounds(pieceIndex, block);

  if (blockStart + block.length > totalFileSize()) {
    throw std::runtime_error(
        "Block extends beyond the end of the managed file data.");
  }

  std::vector<char> data(block.length, 0); // Initialize vector with zeros

  size_t dataOffset = 0; // Offset within the data vector

  for (const auto &file : files_) {
    if (blockEnd <= file.startOffset || blockStart >= file.endOffset)
      continue; // Block does not intersect with this file

    size_t fileReadStart =
        std::max(blockStart, file.startOffset) - file.startOffset;
    size_t blockReadOffset =
        std::max(file.startOffset, blockStart) - blockStart;
    size_t readSize = std::min(file.endOffset, blockEnd) -
                      std::max(file.startOffset, blockStart);

    if (readSize <= 0) {
      continue; // No valid data to read based on calculated size
    }

    std::ifstream fileStream(file.path, std::ios::binary);
    if (!fileStream) {
      throw std::runtime_error("Failed to open file: " + file.path);
    }

    fileStream.seekg(fileReadStart);
    fileStream.read(&data[dataOffset + blockReadOffset], readSize);
    if (fileStream.gcount() != static_cast<std::streamsize>(readSize)) {
      throw std::runtime_error(
          "Failed to read the full expected amount of data from file.");
    }

    dataOffset += readSize;
  }

  return data;
}

void LinuxFileManager::preAllocateSpace() {
  for (const auto &file : files_) {
    int fd = open(file.path.c_str(), O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
      throw std::runtime_error("Failed to open file: " + file.path +
                               "; Error: " + std::string(strerror(errno)));
    }

    if (posix_fallocate(fd, 0, file.length) != 0) {
      close(fd);
      throw std::runtime_error("Failed to pre-allocate file space for " +
                               file.path +
                               "; Error: " + std::string(strerror(errno)));
    }

    close(fd);
  }
}
