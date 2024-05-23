#include "FileManager.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>

FileManager::FileManager(const std::vector<FileInfo> &files,
                         uint32_t pieceLength, std::vector<InfoHash> infoHashes)

    : files_(files), pieceLength_(pieceLength) {
  if (infoHashes.size() != totalPieces()) {
    throw std::invalid_argument("Mismatch in hashes and total pieces");
  }

  // Initialize pieces_ for each piece defined by the info hashes
  for (const auto &infoHash : infoHashes) {
    auto info = std::make_unique<PieceBufferInfo>(pieceLength, infoHash);

    pieces_.push_back(PieceData{nullptr, std::move(info)});
  }
}

LinuxFileManager::LinuxFileManager(const std::vector<FileInfo> &files,
                                   uint32_t pieceLength,
                                   std::vector<InfoHash> infoHashes)
    : FileManager(files, pieceLength, infoHashes) {
  preAllocateSpace();
}

size_t FileManager::totalPieces() const {
  uint64_t totalSize = 0;

  for (const auto &file : files_) {
    totalSize += file.length;
  }

  size_t numPieces = totalSize / pieceLength_;

  if (totalSize % pieceLength_ != 0) {
    ++numPieces;
  }

  return numPieces;
}

bool LinuxFileManager::writeBlock(uint32_t pieceIndex, uint32_t offset,
                                  const std::vector<char> &data) {
  if (pieceIndex >= pieces_.size()) {
    return false;
  }

  PieceData &pieceData = pieces_[pieceIndex];

  if (pieceData.buffer == nullptr) {
    pieceData.buffer = std::make_unique<PieceBuffer>(pieceLength_);
  }

  pieceData.buffer->addData(offset, data);

  pieceData.info->addBlock(offset, data.size());
  if (pieceData.info->isComplete()) {
    std::cout << "Writing complete block" << std::endl;
    return writePiece(pieceIndex);
  }

  return true;
}

std::vector<char> LinuxFileManager::readBlock(uint32_t pieceIndex,
                                              uint32_t offset,
                                              uint32_t length) const {
  std::vector<char> buffer(length);
  uint64_t bytesRead = 0;
  uint64_t globalOffset = static_cast<uint64_t>(
      pieceIndex * pieceLength_ + offset); // Ensure uint64_t for multiplication

  for (const auto &file : files_) {
    if (globalOffset < file.endOffset) {
      uint64_t fileOffset = globalOffset - file.startOffset;
      uint64_t bytesToRead =
          std::min(length - bytesRead, file.length - fileOffset);
      int fd = open(file.path.c_str(), O_RDONLY);
      if (fd == -1) {
        std::cerr << "Failed to open file for reading: " << strerror(errno)
                  << std::endl;
        return {};
      }

      lseek(fd, fileOffset, SEEK_SET);
      ssize_t result = read(fd, buffer.data() + bytesRead, bytesToRead);
      close(fd);

      if (result == -1) {
        std::cerr << "Failed to read file: " << strerror(errno) << std::endl;
        return {};
      }

      bytesRead += result;
      globalOffset += result;
      if (bytesRead == length) {
        break;
      }
    }
  }

  return buffer;
}

void LinuxFileManager::preAllocateSpace() {
  for (const auto &file : files_) {
    int fd = open(file.path.c_str(), O_WRONLY | O_CREAT, 0666);
    if (fd == -1) {
      std::cerr << "Failed to open file for pre-allocation: " << strerror(errno)
                << std::endl;
      continue;
    }

    if (ftruncate(fd, file.length) == -1) {
      std::cerr << "Failed to pre-allocate space: " << strerror(errno)
                << std::endl;
    }
    close(fd);
  }
}

bool LinuxFileManager::writePiece(uint32_t pieceIndex) {
  if (pieceIndex >= pieces_.size()) {
    return false;
  }

  const PieceData &pieceData = pieces_[pieceIndex];
  const std::vector<char> &data = pieceData.buffer->getData();

  uint64_t offset = static_cast<uint64_t>(pieceIndex * pieceLength_);
  uint64_t remainingData = static_cast<uint64_t>(pieceLength_);
  size_t dataOffset = 0;

  for (const auto &file : files_) {
    if (offset < file.endOffset) {
      uint64_t fileOffset = offset - file.startOffset;
      uint64_t writeSize = std::min(remainingData, file.length - fileOffset);
      if (!writeToFile(file.path, fileOffset, &data[dataOffset], writeSize)) {
        return false;
      }
      offset += writeSize;
      remainingData -= writeSize;
      dataOffset += writeSize;
      if (remainingData == 0) {
        break;
      }
    }
  }

  return true;
}

bool LinuxFileManager::writeToFile(const std::string &path, uint64_t offset,
                                   const char *data, uint64_t length) {
  int fd = open(path.c_str(), O_WRONLY);
  if (fd == -1) {
    std::cerr << "Failed to open file for writing: " << strerror(errno)
              << std::endl;
    return false;
  }

  lseek(fd, offset, SEEK_SET);
  ssize_t bytesWritten = write(fd, data, length);
  close(fd);

  if (bytesWritten != length) {
    std::cerr << "Failed to write file: " << strerror(errno) << std::endl;
    return false;
  }

  return true;
}

bool LinuxFileManager::writeTestPiece(const char *data, uint32_t offset,
                                      uint32_t length) {
  return writeToFile(files_[0].path, static_cast<uint64_t>(offset), data,
                     (length));
}
