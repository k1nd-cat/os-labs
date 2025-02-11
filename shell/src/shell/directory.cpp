#include "directory.h"
#include <stdexcept>

std::string Directory::getCurrentDirectory() {
    if (GetCurrentDirectoryA(MAX_PATH, current_dir_) != 0)
        return current_dir_;
    throw std::runtime_error("Error while retrieving the current directory");
}

void Directory::setDirectory(const std::string&  newPath) {
    if (SetCurrentDirectoryA(newPath.c_str())) return;
    throw std::runtime_error("Failed to find the directory: " + newPath);
}

std::vector<std::string> Directory::listContents() const {
    std::vector<std::string> contents;
    WIN32_FIND_DATAA findData;
    HANDLE hFind;

    std::string searchPath = std::string(current_dir_) + "\\*";

    hFind = FindFirstFileA(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to open directory for listing");
    }

    do {
        if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) {
            continue;
        }

        contents.emplace_back(findData.cFileName);
    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);

    return contents;
}
