#include "Torrent.h"

Torrent::Torrent() {}

bool Torrent::isSingleFile() const { return files.size() == 1; }
