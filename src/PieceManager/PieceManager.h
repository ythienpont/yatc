#ifndef PIECEMANAGER_H
#define PIECEMANAGER_H

#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <unordered_set>

class PieceManager {
public:
    PieceManager(const std::vector<std::string>& filePaths, size_t pieceSize);

    bool hasPiece(size_t pieceIndex) const;
    std::unordered_set<size_t> missingPieces() const;
    bool savePiece(size_t pieceIndex, const std::vector<char>& data);
    bool checkIntegrity(size_t pieceIndex, const std::string& expectedHash);

private:
    std::vector<std::string> filePaths;
    size_t pieceSize;
    std::vector<bool> downloadedPieces;
    size_t totalPieces;

    size_t calculateTotalPieces() const;
    std::string calculatePieceHash(const std::vector<char>& data) const;
};

#endif // PIECEMANAGER_H

