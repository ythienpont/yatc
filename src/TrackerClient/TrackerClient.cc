#include "TrackerClient.h"

TrackerClient::TrackerClient(const std::string& trackerUrl, 
    const std::array<char, 20>& infoHash, const std::array<char, 20>& peerId,
    uint16_t port, uint64_t uploaded, uint64_t downloaded, uint64_t left)
  : trackerUrl(trackerUrl), infoHash(infoHash), peerId(peerId), port(port),
  uploaded(uploaded), downloaded(downloaded), left(left) {}

std::string TrackerClient::buildQueryString(TrackerClient::Event event) {
    std::stringstream ss;
    ss << trackerUrl << "?"
       << "info_hash=" << arrayToString(infoHash)
       << "&peer_id=" << arrayToString(peerId)
       << "&port=" << port
       << "&uploaded=" << uploaded
       << "&downloaded=" << downloaded
       << "&left=" << left;

      switch (event) {
        case Event::Started:
            ss <<  "&event=started";
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

TrackerClient::TrackerResponse TrackerClient::announce(TrackerClient::Event event) {
  CURL *curl = curl_easy_init();
  if(curl) {
      std::string url = buildQueryString(event);
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      // Handle SSL if necessary
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      CURLcode res = curl_easy_perform(curl);
      if(res != CURLE_OK) {
          fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(res));
      }
      curl_easy_cleanup(curl);
  } else {
      fprintf(stderr, "Failed to initialize libcurl.\n");
  }
  
  return TrackerClient::TrackerResponse();
}

