#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>

#include "bench1.h"

void workerThread(const int bufferSize, const int repetitionsCount, double &totalTime) {
    for (int i = 0; i < repetitionsCount; ++i) {
        totalTime += emaSearchInt(bufferSize);
    }
}

int main(const int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <repetitionsCount> <threadsCount>" << std::endl;
        return 1;
    }

    try {
        int bufferSize = 32 * 1024 * 1024;
        int repetitionsCount = std::stoi(argv[1]);
        const int threadsCount = std::stoi(argv[2]);

        if (repetitionsCount <= 0 || threadsCount <= 0) {
            throw std::invalid_argument("All arguments must be positive integers.");
        }

        std::vector<std::thread> threads;
        std::vector<double> threadTimes(threadsCount, 0.0);

        threads.reserve(threadsCount);
        for (int i = 0; i < threadsCount; ++i) {
            threads.emplace_back(workerThread, bufferSize, repetitionsCount, std::ref(threadTimes[i]));
        }

        for (auto &t: threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        double totalTime = 0.0;
        for (const double time: threadTimes) {
            totalTime += time;
        }

        std::cout << "Total execution time for all threads: " << std::fixed << std::setprecision(6)
                << totalTime << " seconds" << std::endl;
        std::cout << "Average execution time per repetition: " << std::fixed << std::setprecision(6)
                << totalTime / (repetitionsCount * threadsCount) << " seconds" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
