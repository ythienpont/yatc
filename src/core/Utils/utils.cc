#include "utils.h"
#include <iomanip>
#include <iostream>

uint32_t bytes_to_uint32(const std::vector<std::byte> &bytes, size_t offset) {
  uint32_t value = 0;
  for (int i = 0; i < 4; ++i) {
    value |=
        (static_cast<uint32_t>(static_cast<unsigned char>(bytes[offset + i]))
         << (8 * (3 - i)));
  }
  return value;
}

void print_bytes(const std::vector<std::byte> &bytes) {
  for (auto &b : bytes) {
    std::cout << std::hex << std::setw(2) << std::setfill('0')
              << std::to_integer<int>(b) << " ";
  }
  std::cout << std::dec << std::endl;
}
