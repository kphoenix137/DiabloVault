// exepath.cpp

#include "util/exepath.h"

#include <filesystem>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace fs = std::filesystem;

namespace dv::util {

std::string GetExecutableDir()
{
	fs::path exePath;

#if defined(_WIN32)
	wchar_t buf[MAX_PATH];
	DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
	if (len > 0)
		exePath = fs::path(buf);
#elif defined(__linux__)
	char buf[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (len > 0) {
		buf[len] = '\0';
		exePath = fs::path(buf);
	}
#elif defined(__APPLE__)
	uint32_t size = 0;
	_getExecutablePath(nullptr, &size);
	std::string tmp(size, '\0');
	if (_NSGetExecutablePath(tmp.data(), &size) == 0)
		exePath = fs::path(tmp.c_str());
#endif

	if (exePath.empty()) {
		// Fallback: current working directory.
		exePath = fs::current_path();
		// Avoid std::u8string (char8_t) under /std:c++20.
		return exePath.lexically_normal().string();
	}
	return exePath.parent_path().lexically_normal().string();
}

std::string GetDefaultTxtdataDir()
{
	fs::path p = fs::path(GetExecutableDir()) / ".." / "assets" / "txtdata";
	// Avoid std::u8string (char8_t) under /std:c++20.
	return p.lexically_normal().string();
}

} // namespace dv::util
