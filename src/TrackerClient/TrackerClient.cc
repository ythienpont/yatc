#include "TrackerClient.h"
#include "../../lib/bencode.hpp"
#include <iostream>
#include <random>

size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

const std::string TrackerClient::PREFIX = "-YATC-";

std::array<std::byte, 20> generatePeerId() {
  std::array<std::byte, 20> peerId{};

  // Copy the prefix into the beginning of peerId
  std::transform(TrackerClient::PREFIX.begin(), TrackerClient::PREFIX.end(),
                 peerId.begin(), [](char c) {
                   return std::byte{static_cast<unsigned char>(c)};
                 });

  // Random device and generator for the remaining bytes
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 255);

  // Fill the rest of the array with random bytes
  for (size_t i = TrackerClient::PREFIX.size(); i < peerId.size(); ++i) {
    peerId[i] = static_cast<std::byte>(dis(gen));
  }

  return peerId;
}

std::string urlEncode(const std::array<std::byte, 20> &data) {
  std::ostringstream encodedStream;
  for (auto byte : data) {
    unsigned char unsignedByte = std::to_integer<unsigned char>(byte);
    if (isalnum(unsignedByte) || unsignedByte == '-' || unsignedByte == '_' ||
        unsignedByte == '.' || unsignedByte == '~') {
      encodedStream << static_cast<char>(unsignedByte);
    } else {
      encodedStream << '%' << std::uppercase << std::setw(2)
                    << std::setfill('0') << std::hex
                    << static_cast<int>(unsignedByte);
    }
  }

  return encodedStream.str();
}

std::string arrayToHexString(const std::array<char, 20> &data) {
  std::stringstream hexStream;

  hexStream << std::hex << std::setfill('0');

  for (unsigned char byte : data) {
    hexStream << std::setw(2) << (int)byte;
  }

  return hexStream.str();
}

TrackerClient::TrackerClient(const Torrent &torrent, const uint16_t port)
    : torrent_(torrent), port_(port) {
  peerId_ = generatePeerId();
}

std::string TrackerClient::buildQueryString(TrackerClient::Event event) const {
  std::stringstream ss;
  ss << torrent_.trackerUrl << "?"
     << "info_hash=" << urlEncode(torrent_.infoHash)
     << "&peer_id=" << urlEncode(peerId_) << "&port=" << port_
     << "&uploaded=" << uploaded_ << "&downloaded=" << downloaded_
     << "&left=" << left_;

  switch (event) {
  case Event::Started:
    ss << "&event=started";
    break;
  case Event::Completed:
    ss << "&event=completed";
    break;
  case Event::Stopped:
    ss << "&event=stopped";
    break;
  case Event::Empty:
  default:
    break;
  }

  return ss.str();
}

TrackerResponse parseTrackerResponse(const std::string &readBuffer) {
  TrackerResponse response;
  try {
    auto data = bencode::decode(readBuffer);
    auto dict = std::get<bencode::dict>(
        data); // Assuming a successful decode gives a dict

    // Check for failure reason
    if (dict.find("failure reason") != dict.end()) {
      response.failureReason = std::get<std::string>(dict["failure reason"]);
      return response;
    }

    // Parse interval
    if (dict.find("interval") != dict.end()) {
      response.interval =
          static_cast<uint16_t>(std::get<bencode::integer>(dict["interval"]));
    }

    // Parse peers
    if (dict.find("peers") != dict.end()) {
      auto peersList = std::get<bencode::list>(dict["peers"]);
      for (auto &peerDictVariant : peersList) {
        auto peerDict = std::get<bencode::dict>(peerDictVariant);

        Peer peer;
        if (peerDict.find("ip") != peerDict.end()) {
          peer.ip = std::get<std::string>(peerDict["ip"]);
        }
        if (peerDict.find("port") != peerDict.end()) {
          peer.port = static_cast<uint16_t>(
              std::get<bencode::integer>(peerDict["port"]));
        }

        // Assuming you have a way to generate or retrieve a unique PeerId for
        // each peer
        PeerId peerId = generatePeerId(); // Implement this function based on
                                          // your requirements
        response.peers[peerId] = peer;
      }
    }
  } catch (const bencode::decode_error &e) {
    std::cerr << "Failed to parse tracker response: " << e.what() << std::endl;
    // Handle error, potentially setting a failure reason or re-throwing
  }

  return response;
}

TrackerResponse TrackerClient::announce(TrackerClient::Event event) {
  CURL *curl = curl_easy_init();
  std::string readBuffer;
  if (curl) {
    std::string url = buildQueryString(event);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    // Handle SSL if necessary
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }
    curl_easy_cleanup(curl);
  } else {
    fprintf(stderr, "Failed to initialize libcurl.\n");
  }
  return parseTrackerResponse(readBuffer);
}

std::string TrackerResponse::toString() const {
  std::string response;
  for (auto const &p : peers) {
    response += p.second.ip;
    response += " listening at ";
    response += std::to_string(p.second.port);
    response += "\n";
  }

  return response;
}
