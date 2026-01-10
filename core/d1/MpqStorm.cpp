#include "MpqStorm.h"

#include <cstring>

#ifdef DIABLOVAULT_ENABLE_STORMLIB
#include <StormLib.h>
#endif

namespace dv::d1 {

bool ReadMpqFileStorm(const std::string &mpqPath, const std::string &internalName, std::vector<std::byte> &out, std::string &err)
{
#ifndef DIABLOVAULT_ENABLE_STORMLIB
    (void)mpqPath;
    (void)internalName;
    (void)out;
    err = "StormLib support is disabled (DIABLOVAULT_ENABLE_STORMLIB=OFF)";
    return false;
#else
    HANDLE hArchive = nullptr;
    if (!SFileOpenArchive(mpqPath.c_str(), 0, STREAM_FLAG_READ_ONLY, &hArchive)) {
        err = "SFileOpenArchive failed";
        return false;
    }

    HANDLE hFile = nullptr;
    if (!SFileOpenFileEx(hArchive, internalName.c_str(), 0, &hFile)) {
        SFileCloseArchive(hArchive);
        err = "SFileOpenFileEx failed for: " + internalName;
        return false;
    }

    const DWORD fileSize = SFileGetFileSize(hFile, nullptr);
    if (fileSize == SFILE_INVALID_SIZE) {
        SFileCloseFile(hFile);
        SFileCloseArchive(hArchive);
        err = "SFileGetFileSize failed";
        return false;
    }

    out.resize(fileSize);
    DWORD bytesRead = 0;
    const BOOL ok = SFileReadFile(hFile, out.data(), fileSize, &bytesRead, nullptr);
    SFileCloseFile(hFile);
    SFileCloseArchive(hArchive);

    if (!ok || bytesRead != fileSize) {
        err = "SFileReadFile failed";
        out.clear();
        return false;
    }

    return true;
#endif
}

} // namespace dv::d1
