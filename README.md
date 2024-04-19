# Yet Another Torrent Client (YATC)

![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Dependencies](https://img.shields.io/badge/dependencies-curl%20|%20openssl-blue.svg)

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

- `curl`
- `openssl`

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
./yatc [options] <torrent-file>
```

Replace [options] with your preferred command-line options and `<torrent-file>` with the path to a .torrent file.

## Contributing

Contributions are what make the open-source community such a fantastic place to learn, inspire, and create. Any contributions you make are greatly appreciated.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".

1.  Fork the Project
2.  Create your Feature Branch (git checkout -b feature/AmazingFeature)
3.  Commit your Changes (git commit -m 'Add some AmazingFeature')
4.  Push to the Branch (git push origin feature/AmazingFeature)
5.  Open a Pull Request

## License
Distributed under the MIT License. 
