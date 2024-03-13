#include "PeerConnection.h"

void PeerConnection::handshake() {
  tcp::resolver resolver(ioContext_);
  auto endpoints = resolver.resolve(ip_, std::to_string(port_));
  boost::asio::async_connect(socket_, endpoints,
                             boost::bind(&PeerConnection::handle_connect, this,
                                         boost::asio::placeholders::error));
}

void PeerConnection::handle_connect(const boost::system::error_code &error) {
  if (!error) {
    // Successfully connected, now send handshake
    // Construct handshake message in write_buffer_
    // ...

    boost::asio::async_write(socket_, boost::asio::buffer(writeBuffer_),
                             boost::bind(&PeerConnection::handle_write, this,
                                         boost::asio::placeholders::error));
  } else {
    std::cerr << "Connect error: " << error.message() << std::endl;
  }
}

void PeerConnection::handle_write(const boost::system::error_code &error) {
  if (!error) {
    // Handshake sent, now read response
    boost::asio::async_read(
        socket_, boost::asio::buffer(readBuffer_),
        boost::bind(&PeerConnection::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
  } else {
    std::cerr << "Write error: " << error.message() << std::endl;
  }
}

void PeerConnection::handle_read(const boost::system::error_code &error,
                                 size_t bytes_transferred) {
  if (!error) {
    // Response received, process it
    // ...
    std::cout << bytes_transferred << std::endl;
  } else {
    std::cerr << "Read error: " << error.message() << std::endl;
  }
}
