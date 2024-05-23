#include "TrackerClient.h"
#include <iomanip>
#include <iostream>
#include <random>

size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

std::array<std::byte, 20> stringToByteArray(const std::string &str) {
  if (str.size() != 20) {
    throw std::invalid_argument("String must be exactly 20 bytes long.");
  }

  std::array<std::byte, 20> byteArray;
  for (size_t i = 0; i < 20; ++i) {
    byteArray[i] = static_cast<std::byte>(str[i]);
  }

  return byteArray;
}

const std::string TrackerClient::PREFIX = "-YATC-";

Peer::Id generatePeerId() {
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

std::string arrayToString(const std::array<std::byte, 20> &data) {
  std::stringstream ss;

  for (auto byte : data) {
    ss << (char)byte;
  }

  return ss.str();
}

TrackerClient::TrackerClient(const Torrent &torrent, const uint16_t port)
    : torrent_(torrent), port_(port), uploaded_(0), downloaded_(0) {
  peerId_ = generatePeerId();
  left_ = torrent_.size();
  logger_ = Logger::instance();
  logger_->log("TrackerClient constructed with generated peer ID",
               Logger::DEBUG);
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
    logger_->log("Building query string for event: started", Logger::INFO);
    break;
  case Event::Completed:
    ss << "&event=completed";
    logger_->log("Building query string for event: completed", Logger::INFO);
    break;
  case Event::Stopped:
    ss << "&event=stopped";
    logger_->log("Building query string for event: stopped", Logger::INFO);
    break;
  case Event::Empty:
  default:
    logger_->log("Building query string with no specific event",
                 Logger::WARNING);
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
      auto peersVariant = dict["peers"];
      if (peersVariant.index() ==
          1) { // Check if it is a string (compact format)
        auto peersString = std::get<std::string>(peersVariant);
        std::cout << peersString << std::endl;
        for (size_t i = 0; i < peersString.size(); i += 6) {
          Peer peer;
          struct in_addr ip_addr;
          std::memcpy(&ip_addr, peersString.data() + i, 4);
          peer.ip = inet_ntoa(ip_addr);
          peer.port = ntohs(
              *reinterpret_cast<const uint16_t *>(peersString.data() + i + 4));
          std::cout << "Peer IP: " << peer.ip << ", Port: " << peer.port
                    << std::endl;
          response.peers.emplace_back(peer);
        }
      } else if (peersVariant.index() == 2) { // List of dictionaries format
        auto peersList = std::get<bencode::list>(peersVariant);
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
          if (peerDict.find("peer id") != peerDict.end()) {
            peer.id =
                stringToByteArray(std::get<std::string>(peerDict["peer id"]));
          }

          response.peers.emplace_back(peer);
        }
      }
    }
  } catch (const bencode::decode_error &e) {
    std::cerr << readBuffer << std::endl;
    throw std::runtime_error("Failed to parse tracker response");
  }
  std::cout << "Tracker response parsed\n";
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
    logger_->log("Making HTTP request to tracker URL: " + url, Logger::DEBUG);

    // Handle SSL if necessary
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      logger_->log("curl_easy_perform() failed: " +
                       std::string(curl_easy_strerror(res)),
                   Logger::ERROR);
    }
    curl_easy_cleanup(curl);
  } else {
    logger_->log("Failed to initialize libcurl.", Logger::ERROR);
  }

  return parseTrackerResponse(readBuffer);
}
