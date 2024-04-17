#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "Torrent/Torrent.h"

class FileManager {
public:
  bool writePiece(const size_t pieceIndex, const std::vector<char> &data);
  void readPiece(const size_t pieceIndex) const;

  FileManager(const Torrent &torrent);
  ~FileManager() = default;

private:
  Torrent torrent_;
};

#endif // FILEMANAGER_H
