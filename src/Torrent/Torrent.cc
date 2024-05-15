#include "Torrent.h"
#include <cstdint>

bool Torrent::isSingleFile() const { return files.size() == 1; }

size_t Torrent::totalPieces() const { return pieces.size(); }

uint64_t Torrent::size() const {
  uint64_t size = 0;
  for (const auto &file : files)
    size += file.length;

  return size;
}
