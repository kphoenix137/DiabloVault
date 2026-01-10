#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace dv::d1 {

// Reads an internal file from an MPQ archive using StormLib.
// Returns true on success and fills 'out'. On failure returns false and fills 'err'.
bool ReadMpqFileStorm(const std::string &mpqPath, const std::string &internalName, std::vector<std::byte> &out, std::string &err);

} // namespace dv::d1
