#include "FileManager.h"
#include <fstream>

FileManager::FileManager(const Torrent &torrent) : torrent_(torrent) {}

bool FileManager::writePiece(size_t pieceIndex, const std::vector<char> &data) {
  if (pieceIndex >= torrent_.totalPieces())
    return false;

  // Assuming single file for simplicity
  std::fstream file(torrent_.files[0].path,
                    std::ios::binary | std::ios::out | std::ios::in);
  if (!file)
    return false;

  file.seekp(pieceIndex * torrent_.pieceLength);
  file.write(data.data(), data.size());
  if (file.fail())
    return false;

  return true;
}

void FileManager::readPiece(const size_t pieceIndex) const {
  throw std::logic_error("Not implemented");
}
