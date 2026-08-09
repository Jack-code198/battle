#include "config.h"
#include <cstdio>
#include <cstring>

static bool PathHasMakotoAssets(const char* dir) {
    if (!dir || !dir[0]) return false;

    char path[MAX_PATH];
    sprintf_s(path, "%s\\assets\\makoto\\stance.png", dir);
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

void EnsureGameWorkingDirectory() {
    char exeDir[MAX_PATH];
    GetModuleFileNameA(NULL, exeDir, MAX_PATH);

    char* slash = strrchr(exeDir, '\\');
    if (slash) {
        *slash = '\0';
    }

    const char* relativeCandidates[] = { "", "..", "..\\.." };
    for (const char* rel : relativeCandidates) {
        char candidate[MAX_PATH];
        if (!rel[0]) {
            strcpy_s(candidate, exeDir);
        }
        else {
            sprintf_s(candidate, "%s\\%s", exeDir, rel);
        }

        char normalized[MAX_PATH];
        DWORD len = GetFullPathNameA(candidate, MAX_PATH, normalized, NULL);
        if (len == 0 || len >= MAX_PATH) {
            continue;
        }

        if (PathHasMakotoAssets(normalized)) {
            SetCurrentDirectoryA(normalized);
            return;
        }
    }
}
