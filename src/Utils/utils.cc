#include "utils.h"

uint32_t bytes_to_uint32(const std::vector<std::byte> &bytes, size_t offset) {
  uint32_t value = 0;
  for (int i = 0; i < 4; ++i) {
    value |=
        (static_cast<uint32_t>(static_cast<unsigned char>(bytes[offset + i]))
         << (8 * (3 - i)));
  }
  return value;
}
