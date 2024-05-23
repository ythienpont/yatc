#include <string>
#include <vector>

enum class PieceState { Left, Started, Downloaded };

class TorrentInfo {
public:
  std::string name;
  std::string eta;
  std::vector<PieceState> pieces;

  TorrentInfo(const std::string &name, const std::string &eta, int totalPieces);
  void initializePieces(int totalPieces);
  void updatePieces();
  void updateEta(const std::string &newEstimate);
};
