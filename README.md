# Yet Another Torrent Client (YATC)

![License](https://img.shields.io/badge/license-MIT-green.svg)

Yet Another Torrent Client (YATC) is a lightweight, efficient torrent client designed for simplicity and speed, with support for extensions.
## Features

- **Lightweight and Fast**: Designed to run smoothly on all platforms.
- **Easy to Use**: Simple setup and intuitive command-line interface.
- **Secure**: Uses OpenSSL for secure connections.
- **Cross-Platform**: Build and run on any Unix-like operating system.
- **Extensible**: Easy to understand codebase allows for easy implementation of any BEP.

## Getting Started

### Prerequisites

Before installing YATC, make sure you have the following dependencies installed on your system:

- `libcurl`
- `openssl`
- `gtk4`
- `Boost.Asio`

### Building
To install YATC, clone the repository and use `make` to build the project:

```bash
git clone https://github.com/ythienpont/yatc.git
cd yatc
make
```

## Usage

To start using YATC, simply run the compiled binary from the terminal:

```bash
./yatc
```

## Documentation
For a detailed overview of each function, you can open the Doxygen documentation at `docs/html/index.html` in your browser.

## License
Distributed under the MIT License. 
