#include "Torrent.h"
#include <cstdint>
#include <iomanip> // For std::setw and std::setfill
#include <sstream> // For std::ostringstream

bool Torrent::isSingleFile() const { return files.size() == 1; }

size_t Torrent::totalPieces() const { return pieces.size(); }

uint64_t Torrent::size() const {
  uint64_t size = 0;
  for (const auto &file : files)
    size += file.length;

  return size;
}

/**
 * @brief Returns a diagnostic string containing detailed information about the
 * torrent.
 *
 * @return std::string A formatted string containing key details of the torrent.
 */
std::string Torrent::diagnosticInfo() const {
  std::ostringstream stream;
  stream << "Torrent Name: " << name << "\n"
         << "Tracker URL: " << trackerUrl << "\n"
         << "Total Size: " << size() << " bytes\n"
         << "Piece Length: " << pieceLength << " bytes\n"
         << "Total Pieces: " << totalPieces() << "\n"
         << "File Count: " << files.size() << "\n"
         << "Single File Torrent: " << (isSingleFile() ? "Yes" : "No") << "\n";

  // Optionally, list the files included in the torrent
  if (!isSingleFile()) {
    stream << "Files in Torrent:\n";
    for (const auto &file : files) {
      stream << " - " << file.path << " (Size: " << file.length << " bytes)\n";
    }
  }

  // Output the InfoHash in a human-readable hexadecimal format
  stream << "InfoHash: ";
  for (auto &byte : infoHash) {
    stream << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<unsigned>(std::to_integer<int>(byte));
  }
  stream << std::dec << "\n"; // Switch back to decimal output

  return stream.str();
}
