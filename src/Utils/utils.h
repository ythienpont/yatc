#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <vector>

uint32_t bytes_to_uint32(const std::vector<std::byte> &bytes,
                         size_t offset = 0);

#endif //! UTILS_H
