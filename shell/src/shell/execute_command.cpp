#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <chrono>

#include "execute_command.h"

std::vector<std::string> split_string(const std::string& input) {
    std::istringstream stream(input);
    std::vector<std::string> words;
    std::string word;

    while (stream >> word) {
        words.push_back(word);
    }

    return words;
}

ULONGLONG run_program(const std::string& programPathWithArguments) {
    std::string path = programPathWithArguments;

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    FILETIME startTime, endTime;
    GetSystemTimeAsFileTime(&startTime);

    if (CreateProcess(
            nullptr,
            &path[0],
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi
    )) {

        WaitForInputIdle(pi.hProcess, INFINITE);
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetSystemTimeAsFileTime(&endTime);

        ULARGE_INTEGER start, end;
        start.LowPart = startTime.dwLowDateTime;
        start.HighPart = startTime.dwHighDateTime;
        end.LowPart = endTime.dwLowDateTime;
        end.HighPart = endTime.dwHighDateTime;

        ULONGLONG elapsed = end.QuadPart - start.QuadPart;

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return elapsed / 10000;
    } else {
        throw std::runtime_error("Error when starting a process");
    }
}

void run_program_as_user(const std::string &programPathWithArguments) {
    const std::string& path = programPathWithArguments;

    HANDLE hToken = nullptr;
    HANDLE hDupToken = nullptr;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken)) {
        std::cerr << "OpenProcessToken failed: " << GetLastError() << std::endl;
        return;
    }

    if (!DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, nullptr, SecurityImpersonation, TokenPrimary, &hDupToken)) {
        std::cerr << "DuplicateTokenEx failed: " << GetLastError() << std::endl;
        CloseHandle(hToken);
        return;
    }

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    auto start = std::chrono::high_resolution_clock::now();

    if (!CreateProcessAsUserA(
            hDupToken,          // Токен пользователя
            nullptr,            // Имя исполняемого модуля (используем командную строку)
            const_cast<char*>(path.c_str()), // Командная строка
            nullptr,            // Атрибуты безопасности процесса
            nullptr,            // Атрибуты безопасности потока
            FALSE,              // Наследование дескрипторов
            0,                  // Флаги создания
            nullptr,            // Окружение (используем родительское)
            nullptr,            // Текущий каталог (используем родительский)
            &si,                // STARTUPINFO
            &pi                 // PROCESS_INFORMATION
    )) {
//        std::cerr << "CreateProcessAsUser failed: " << GetLastError() << std::endl;
        CloseHandle(hDupToken);
        CloseHandle(hToken);
        throw std::runtime_error("Error when starting a process");
//        return;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hDupToken);
    CloseHandle(hToken);

    std::cout << "Process was active for " << elapsed.count() << " seconds." << std::endl;
}

void execute_command(const std::string& command, Directory& directory) {
    auto args = split_string(command);
    if (args.empty()) {
        std::cout << std::endl;
        return;
    }

    if (args[0] == "cls") {
        if (args.size() != 1) {
            std::cerr << "Command must not contain arguments";
            std::cerr.flush();
        } else {
            system("cls");
            return;
        }
    }

    if (args[0] == "dir") {
        if (args.size() != 1) {
            std::cerr << "Command must not contain arguments";
            std::cerr.flush();
        } else {
            auto dirs = directory.listContents();
            for (const auto& dir : dirs) {
                std::cout << dir << std::endl;
            }
        }

        std::cout << std::endl;
        return;
    }

    if (args[0] == "cd") {
        if (args.size() != 2) {
            std::cerr << "Command must contain one argument" << std::endl;
            std::cerr.flush();
        } else {
            try {
                Directory::setDirectory(args[1]);
            } catch (const std::exception &e) {
                std::cerr << e.what() << std::endl;
                std::cerr.flush();
            }
        }

        std::cout << std::endl;
        return;
    }

    try {
        run_program_as_user(command);
        std::cout << std::endl;
        return;
    } catch (const std::exception &ignore) {}

    std::cerr << "'" + command + "'" + " is not an internal or external command, executable programme or batch file." << std::endl;
    std::cerr.flush();
    std::cout << std::endl;
}
