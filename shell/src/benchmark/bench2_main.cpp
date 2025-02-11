#include <iostream>
#include <thread>
#include <vector>
#include <iomanip>

#include "bench2.h"

void workerThread(const std::vector<std::vector<int>> &graph, const int &startNode, const int &repetitionsCount, double &totalTime) {
    totalTime += measureShortPathTime(graph, startNode, repetitionsCount);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <mode> <repetitionsCount> <threadsCount>" << std::endl;
        return 1;
    }

    try {
        int mode = std::stoi(argv[1]);
        int repetitionsCount = std::stoi(argv[2]);
        int threadsCount = std::stoi(argv[3]);

        if (mode != 1 && mode != 2 || repetitionsCount <= 0 || threadsCount <= 0) {
            throw std::invalid_argument("Invalid arguments.");
        }

        std::vector<std::vector<int>> graph;
        int startNode;

        if (mode == 1) {
            graph = shortPathCustom();
            std::cout << "Enter start node: ";
            std::cin >> startNode;
        } else if (mode == 2) {
            graph = shortPathDefault();
            startNode = 0;
        }

        std::vector<std::thread> threads;
        std::vector<double> threadTimes(threadsCount, 0.0);

        for (int i = 0; i < threadsCount; ++i) {
            threads.emplace_back(workerThread, std::ref(graph), startNode, repetitionsCount, std::ref(threadTimes[i]));
        }

        for (auto &t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        double totalTime = 0.0;
        for (const double time : threadTimes) {
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