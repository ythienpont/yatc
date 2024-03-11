#include "TrackerClient.h"
#include <iostream>
#include <random>

const std::string TrackerClient::PREFIX = "-YATC-";

std::array<char, 20> generatePeerID() {
  std::array<char, 20> peerID{};

  std::copy(TrackerClient::PREFIX.begin(), TrackerClient::PREFIX.end(),
            peerID.begin());

  // Random device and generator
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 255);

  // Fill the rest of the array with random bytes
  for (size_t i = TrackerClient::PREFIX.size(); i < peerID.size(); ++i) {
    peerID[i] = static_cast<char>(dis(gen));
  }

  return peerID;
}

std::string arrayToHexString(const std::array<char, 20> &data) {
  std::stringstream hexStream;

  hexStream << std::hex << std::setfill('0');

  for (unsigned char byte : data) {
    hexStream << std::setw(2) << (int)byte;
  }

  return hexStream.str();
}

TrackerClient::TrackerClient(const std::string &trackerUrl,
                             const std::array<char, 20> &infoHash,
                             const uint64_t left, const uint16_t port,
                             const uint64_t uploaded, const uint64_t downloaded)
    : trackerUrl(trackerUrl), infoHash(infoHash), port(port),
      uploaded(uploaded), downloaded(downloaded), left(left) {
  peerId = generatePeerID();
}

std::string TrackerClient::buildQueryString(TrackerClient::Event event) const {
  std::stringstream ss;
  ss << trackerUrl << "?"
     << "info_hash=" << arrayToHexString(infoHash)
     << "&peer_id=" << arrayToHexString(peerId) << "&port=" << port
     << "&uploaded=" << uploaded << "&downloaded=" << downloaded
     << "&left=" << left;

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

  std::cout << ss.str() << std::endl;

  return ss.str();
}

TrackerClient::TrackerResponse
TrackerClient::announce(TrackerClient::Event event) {
  CURL *curl = curl_easy_init();
  if (curl) {
    std::string url = buildQueryString(event);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

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

  return TrackerClient::TrackerResponse();
}
