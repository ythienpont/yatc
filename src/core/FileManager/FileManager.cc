#include "FileManager.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
/*

FileManager::FileManager(const std::vector<FileInfo> &files,
                         uint32_t piece_length,
                         std::vector<InfoHash> info_hashes)

    : files_(files), piece_length_(piece_length) {
  if (info_hashes.size() != total_pieces()) {
    throw std::invalid_argument("Mismatch in hashes and total pieces");
  }

  // Initialize pieces_ for each piece defined by the info hashes
  for (const auto &info_hash : info_hashes) {
    auto info = std::make_unique<PieceBufferInfo>(piece_length, info_hash);

    pieces_.push_back(PieceBuffer{nullptr, std::move(info)});
  }
}

LinuxFileManager::LinuxFileManager(const std::vector<FileInfo> &files,
                                   uint32_t piece_length,
                                   std::vector<InfoHash> info_hashes)
    : FileManager(files, piece_length, info_hashes) {
  pre_allocate_space();
}

uint32_t FileManager::total_pieces() const {
  uint64_t total_size = 0;

  for (const auto &file : files_) {
    total_size += file.length;
  }

  size_t num_pieces = total_size / piece_length_;

  if (total_size % piece_length_ != 0) {
    ++num_pieces;
  }

  return num_pieces;
}

bool FileManager::write_block(uint32_t piece_index, uint32_t offset,
                              const std::vector<std::byte> &data) {
  if (piece_index >= pieces_.size()) {
    return false;
  }

  PieceBuffer &piece_buffer = pieces_[piece_index];

  if (piece_buffer.buffer == nullptr) {
    piece_buffer.buffer = std::make_unique<PieceBufferData>(piece_length_);
  }

  piece_buffer.buffer->add_data(offset, data);

  piece_buffer.info->add_block(offset, data.size());
  if (piece_buffer.info->is_complete()) {
    std::cout << "Writing complete block" << std::endl;
    return write_piece(piece_index);
  }

  return true;
}

std::vector<std::byte> LinuxFileManager::read_block(uint32_t piece_index,
                                                    uint32_t offset,
                                                    uint32_t length) const {
  std::vector<std::byte> buffer(length);
  uint64_t bytes_read = 0;
  uint64_t global_offset =
      static_cast<uint64_t>(piece_index * piece_length_ +
                            offset); // Ensure uint64_t for multiplication

  for (const auto &file : files_) {
    if (global_offset < file.end_offset) {
      uint64_t file_offset = global_offset - file.start_offset;
      uint64_t bytes_to_read =
          std::min(length - bytes_read, file.length - file_offset);
      int fd = open(file.path.c_str(), O_RDONLY);
      if (fd == -1) {
        std::cerr << "Failed to open file for reading: " << strerror(errno)
                  << std::endl;
        return {};
      }

      lseek(fd, file_offset, SEEK_SET);
      ssize_t result =
          read(fd, reinterpret_cast<char *>(buffer.data()) + bytes_read,
               bytes_to_read);
      close(fd);

      if (result == -1) {
        std::cerr << "Failed to read file: " << strerror(errno) << std::endl;
        return {};
      }

      bytes_read += result;
      global_offset += result;
      if (bytes_read == length) {
        break;
      }
    }
  }

  return buffer;
}

void LinuxFileManager::pre_allocate_space() {
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

bool LinuxFileManager::write_piece(uint32_t piece_index) {
  if (piece_index >= pieces_.size()) {
    return false;
  }

  const PieceBuffer &piece_buffer = pieces_[piece_index];
  const std::vector<std::byte> &data = piece_buffer.buffer->get_data();

  uint64_t offset = static_cast<uint64_t>(piece_index * piece_length_);
  uint64_t remaining_data = static_cast<uint64_t>(piece_length_);
  size_t data_offset = 0;

  for (const auto &file : files_) {
    if (offset < file.end_offset) {
      uint64_t file_offset = offset - file.start_offset;
      uint64_t write_size = std::min(remaining_data, file.length - file_offset);
      if (!write_to_file(file.path, file_offset, &data[data_offset],
                         write_size)) {
        return false;
      }
      offset += write_size;
      remaining_data -= write_size;
      data_offset += write_size;
      if (remaining_data == 0) {
        break;
      }
    }
  }

  return true;
}

bool LinuxFileManager::write_to_file(const std::string &path, uint64_t offset,
                                     const std::byte *data, uint64_t length) {
  int fd = open(path.c_str(), O_WRONLY);
  if (fd == -1) {
    std::cerr << "Failed to open file for writing: " << strerror(errno)
              << std::endl;
    return false;
  }

  lseek(fd, offset, SEEK_SET);
  ssize_t bytes_written = write(fd, data, length);
  close(fd);

  if (bytes_written != length) {
    std::cerr << "Failed to write file: " << strerror(errno) << std::endl;
    return false;
  }

  return true;
}

bool LinuxFileManager::write_test_piece(const std::byte *data, uint32_t offset,
                                        uint32_t length) {
  return write_to_file(files_[0].path, static_cast<uint64_t>(offset), data,
                       (length));
}
*/
