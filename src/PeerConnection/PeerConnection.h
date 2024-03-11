#ifndef PEERCONNECTION_H
#define PEERCONNECTION_H

#include <cstdint>
#include <string>
#include <vector>

class PeerConnection {
public:
  PeerConnection(const std::string& ip, uint16_t port, const std::string& peerId)
      : ip(ip), port(port), peerId(peerId), downloaded(0), uploaded(0) {}

  // Establish connection and perform handshake
  bool handshake();

  // Express interest in downloading
  void sendInterest();

  // Request a piece of the file
  void sendRequest(int pieceIndex);

  // Handle receiving a piece of the file
  void receivePiece(int pieceIndex, const std::vector<uint8_t>& pieceData);

  // Close the connection
  void disconnect();

  // Getter for downloaded amount
  uint64_t getDownloaded() const { return downloaded; }

  // Getter for uploaded amount
  uint64_t getUploaded() const { return uploaded; }

private:
  std::string ip;
  uint16_t port;
  std::string peerId;
  std::vector<bool> pieces; // Tracking which pieces this peer has
  uint64_t downloaded; // Total bytes downloaded from this peer
  uint64_t uploaded;   // Total bytes uploaded to this peer

  void updatePiecesAvailability(); // Stub for updating the piece availability from this peer
  bool isConnected(); // Check if the connection is active
};

#endif // PEERCONNECTION_H
