#ifndef UTILS_H
#define UTILS_H

#include <cstddef>
#include <cstdint>
#include <vector>

uint32_t bytes_to_uint32(const std::vector<std::byte> &bytes,
                         size_t offset = 0);

void print_bytes(const std::vector<std::byte> &bytes);
#endif //! UTILS_H
