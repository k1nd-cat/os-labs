#include "bench1.h"

#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>

const std::string RANDOM_NUMBERS_PATH = R"(random_numbers.txt)";

constexpr int TARGET_VALUE = 463361;

bool searchInBuffer(const std::vector<int> &buffer, int target) {
    return std::any_of(buffer.begin(), buffer.end(), [target](const int num) {
        return num == target;
    });
}

void searchInFile(const int &bufferSize) {
    std::ifstream file(RANDOM_NUMBERS_PATH, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file");
    }

    std::vector<int> buffer(bufferSize / sizeof(int));
    int blockNumber = 0;
    while (true) {
        file.read(reinterpret_cast<char *>(buffer.data()), buffer.size() * sizeof(int));
        blockNumber++;
        std::streamsize bytesRead = file.gcount();
        if (bytesRead < buffer.size() * sizeof(int)) {
            const size_t elementsRead = bytesRead / sizeof(int);
            std::fill(buffer.begin() + elementsRead, buffer.end(), 0);
        }

        if (searchInBuffer(buffer, TARGET_VALUE)) {}

        if (file.eof())
            break;
    }
}

double emaSearchInt(const int &bufferSize) {
    const auto start = std::chrono::high_resolution_clock::now();
    searchInFile(bufferSize);
    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> duration = end - start;
    return duration.count();
}

double emaSearchInt(const int &bufferSize, const int &repetitionsCount) {
    double totalTime = 0.0;
    for (int i = 0; i < repetitionsCount; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        searchInFile(bufferSize);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        totalTime += duration.count();
    }
    return totalTime;
}
