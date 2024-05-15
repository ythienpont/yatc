#!/bin/bash

# Create Docker network
docker network create bittorrent-net

# Build Opentracker container
docker build -f dockerfiles/Dockerfile.opentracker -t opentracker .

# Run Opentracker container
docker run -d --name opentracker --network bittorrent-net -p 6969:6969 opentracker

# Prepare the torrent file
echo "Hello BitTorrent" > torrents/sample.txt
# Assume mktorrent is installed locally; adjust as needed
mktorrent -a http://$(docker inspect -f '{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}' opentracker):6969/announce -o torrents/sample.torrent torrents/sample.txt

# Build Peer container
docker build -f dockerfiles/Dockerfile.peer -t bittorrent-peer .

# Run multiple peer instances
docker run -d --name peer1 --network bittorrent-net -v $(pwd)/torrents:/downloads bittorrent-peer
docker run -d --name peer2 --network bittorrent-net -v $(pwd)/torrents:/downloads bittorrent-peer
