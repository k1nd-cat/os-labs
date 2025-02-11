#ifndef SHELL_DIRECTORY_H
#define SHELL_DIRECTORY_H

#include <windows.h>
#include <string>
#include <vector>

class Directory {
public:
    std::string getCurrentDirectory();
    static void setDirectory(const std::string&  newPath);

    [[nodiscard]] std::vector<std::string> listContents() const;

private:
    char current_dir_[MAX_PATH];
};


#endif //SHELL_DIRECTORY_H
