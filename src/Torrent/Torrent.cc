#include "Torrent.h"

bool Torrent::isSingleFile() const { return files.size() == 1; }

size_t Torrent::totalPieces() const { return pieces.size(); }
