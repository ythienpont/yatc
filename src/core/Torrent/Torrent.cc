#include "Torrent.h"
#include <cstdint>
#include <iomanip>
#include <sstream>

bool Torrent::is_single_file() const { return files.size() == 1; }

uint32_t Torrent::total_pieces() const {
  return static_cast<uint32_t>(pieces.size());
}

uint64_t Torrent::size() const {
  uint64_t size = 0;
  for (const auto &file : files)
    size += file.length;

  return size;
}

std::string Torrent::diagnostic_info() const {
  std::ostringstream stream;
  stream << "Torrent Name: " << name << "\n"
         << "Tracker URL: " << tracker_url << "\n"
         << "Total Size: " << size() << " bytes\n"
         << "Piece Length: " << piece_length << " bytes\n"
         << "Total Pieces: " << total_pieces() << "\n"
         << "File Count: " << files.size() << "\n"
         << "Single File Torrent: " << (is_single_file() ? "Yes" : "No")
         << "\n";

  if (!is_single_file()) {
    stream << "Files in Torrent:\n";
    for (const auto &file : files) {
      stream << " - " << file.path << " (Size: " << file.length << " bytes)\n";
    }
  }

  stream << "InfoHash: ";
  for (auto &byte : info_hash) {
    stream << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<unsigned>(std::to_integer<int>(byte));
  }
  stream << std::dec << "\n";

  return stream.str();
}
