#include "FileManager.h"

FileManager::FileManager(const std::vector<FileInfo> &files,
                         uint32_t piece_length,
                         std::vector<InfoHash> info_hashes)

    : files_(files), piece_length_(piece_length) {
  if (info_hashes.size() != total_pieces()) {
    throw std::invalid_argument("Mismatch in hashes and total pieces");
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

std::vector<std::byte> LinuxFileManager::read_block(uint32_t piece_index,
                                                    uint32_t offset,
                                                    uint32_t length) const {
  std::lock_guard<std::mutex> lock(file_mutex_);

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
  std::lock_guard<std::mutex> lock(file_mutex_);

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

bool LinuxFileManager::write_piece(uint32_t piece_index,
                                   std::vector<std::byte> &data) {
  std::lock_guard<std::mutex> lock(file_mutex_);

  if (piece_index >= total_pieces()) {
    std::cerr << "Invalid piece index: " << piece_index << std::endl;
    return false;
  }

  uint64_t offset = static_cast<uint64_t>(piece_index * piece_length_);
  uint64_t remaining_data = static_cast<uint64_t>(piece_length_);
  size_t data_offset = 0;

  for (const auto &file : files_) {

    if (offset < file.end_offset) {
      uint64_t file_offset = offset - file.start_offset;
      uint64_t write_size = std::min(remaining_data, file.length - file_offset);

      std::string data_str(reinterpret_cast<char *>(&data[data_offset]),
                           write_size);

      if (!write_to_file(file.path, file_offset, &data[data_offset],
                         write_size)) {
        std::cerr << "Failed to write to file: " << file.path << std::endl;
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

  if (static_cast<uint64_t>(bytes_written) != length) {
    std::cerr << "Failed to write file: " << strerror(errno) << std::endl;
    return false;
  }

  return true;
}
