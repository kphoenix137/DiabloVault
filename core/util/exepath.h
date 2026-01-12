// exepath.h
#pragma once

#include <string>

namespace dv::util {

// Returns directory containing the current executable (no trailing slash).
std::string GetExecutableDir();

// Convenience: <exeDir>/../assets/txtdata (normalized).
std::string GetDefaultTxtdataDir();

} // namespace dv::util
