#include "Torrent.h"

bool Torrent::isSingleFile() const { return files.size() == 1; }
