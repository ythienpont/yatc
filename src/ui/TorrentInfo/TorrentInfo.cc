#include "TorrentInfo.h"

TorrentInfo::TorrentInfo(const std::string &name, const std::string &eta,
                         int totalPieces)
    : name(name), eta(eta) {
  initializePieces(totalPieces);
}

void TorrentInfo::initializePieces(int totalPieces) {
  pieces.clear();
  for (int i = 0; i < totalPieces; ++i) {
    pieces.push_back(static_cast<PieceState>(rand() % 3));
  }
}

void TorrentInfo::updatePieces() {
  for (auto &piece : pieces) {
    int chance = rand() % 10;
    if (chance < 2) {
      if (piece == PieceState::Left) {
        piece = PieceState::Started;
      } else if (piece == PieceState::Started) {
        piece = PieceState::Downloaded;
      }
    }
  }
}
