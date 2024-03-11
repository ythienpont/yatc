#include "TorrentParser.h"

TorrentParser *TorrentParser::getInstance()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (pinstance == nullptr)
    {
        pinstance = new TorrentParser();
    }
    return pinstance;
}
