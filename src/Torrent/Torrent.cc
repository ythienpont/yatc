#include "Torrent.h"

bool Torrent::isSingleFile() const { return files.size() == 1; }

size_t Torrent::totalFileSize() const {
  size_t totalFileSize = 0;

  for (const auto &file : files) {
    totalFileSize += file.length;
  }

  return totalFileSize;
}

size_t Torrent::totalPieces() const {
  return (totalFileSize() + pieceLength - 1) / pieceLength;
}
