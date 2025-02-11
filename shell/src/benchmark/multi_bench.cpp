#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>
#include "bench1.h"
#include "bench2.h"

void workerThreadShortPath(const std::vector<std::vector<int>> &graph, const int &startNode, const int &repetitionsCount, double &totalTime) {
    totalTime += measureShortPathTime(graph, startNode, repetitionsCount);
}

void workerThreadNumberSearch(const int &bufferSize, const int &repetitionsCount, double &totalTime) {
    totalTime += emaSearchInt(bufferSize, repetitionsCount);
}

int main(const int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <repetitionsCount> <threadsCount>" << std::endl;
        return 1;
    }

    try {
        int repetitionsCount = std::stoi(argv[1]);
        int threadsCount = std::stoi(argv[2]);

        if (repetitionsCount <= 0 || threadsCount <= 0) {
            throw std::invalid_argument("Repetitions count and threads count must be positive integers.");
        }

        const std::vector<std::vector<int>> graph = shortPathDefault();
        const int startNode = 0;

        const int bufferSize = 32 * 1024 * 1024; // 32 MB

        std::vector<std::thread> threadsShortPath;
        std::vector<std::thread> threadsNumberSearch;

        std::vector<double> threadTimesShortPath(threadsCount, 0.0);
        std::vector<double> threadTimesNumberSearch(threadsCount, 0.0);

        for (int i = 0; i < threadsCount; ++i) {
            threadsShortPath.emplace_back(workerThreadShortPath, std::ref(graph), startNode, repetitionsCount, std::ref(threadTimesShortPath[i]));

            threadsNumberSearch.emplace_back(workerThreadNumberSearch, bufferSize, repetitionsCount, std::ref(threadTimesNumberSearch[i]));
        }

        for (auto &t : threadsShortPath) {
            if (t.joinable()) {
                t.join();
            }
        }

        for (auto &t : threadsNumberSearch) {
            if (t.joinable()) {
                t.join();
            }
        }

        double totalTimeShortPath = 0.0;
        for (const double time : threadTimesShortPath) {
            totalTimeShortPath += time;
        }

        double totalTimeNumberSearch = 0.0;
        for (const double time : threadTimesNumberSearch) {
            totalTimeNumberSearch += time;
        }

        std::cout << "Algorithm: Short Path Search" << std::endl;
        std::cout << "Total execution time for all threads: " << std::fixed << std::setprecision(6)
                  << totalTimeShortPath << " seconds" << std::endl;
        std::cout << "Average execution time per repetition: " << std::fixed << std::setprecision(6)
                  << totalTimeShortPath / (repetitionsCount * threadsCount) << " seconds" << std::endl;

        std::cout << "\nAlgorithm: Number Search" << std::endl;
        std::cout << "Total execution time for all threads: " << std::fixed << std::setprecision(6)
                  << totalTimeNumberSearch << " seconds" << std::endl;
        std::cout << "Average execution time per repetition: " << std::fixed << std::setprecision(6)
                  << totalTimeNumberSearch / (repetitionsCount * threadsCount) << " seconds" << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}