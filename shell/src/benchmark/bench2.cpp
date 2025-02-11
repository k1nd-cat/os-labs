#include "bench2.h"

#include <iostream>
#include <chrono>
#include <queue>
#include <limits>
#include <random>

void findShortPath(const std::vector<std::vector<int>> &graph, const int &startNode) {
    const size_t n = graph.size();
    std::vector<int> dist(n, std::numeric_limits<int>::max());
    dist[startNode] = 0;
    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<>> pq;
    pq.emplace(0, startNode);
    while (!pq.empty()) {
        const int u = pq.top().second;
        const int d = pq.top().first;
        pq.pop();
        if (d > dist[u]) continue;
        for (int v = 0; v < n; ++v) {
            if (graph[u][v] != 0) {
                if (int newDist = dist[u] + graph[u][v]; newDist < dist[v]) {
                    dist[v] = newDist;
                    pq.emplace(newDist, v);
                }
            }
        }
    }
}

std::vector<std::vector<int>> generateLargeGraph() {
    constexpr int n = 10000;
    std::vector<std::vector<int>> graph(n, std::vector<int>(n, 0));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> weightDist(1, 10);
    std::uniform_int_distribution<> edgeDist(0, 1);
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (edgeDist(gen) == 1) {
                const int weight = weightDist(gen);
                graph[i][j] = weight;
                graph[j][i] = weight;
            }
        }
    }
    return graph;
}

double measureShortPathTime(const std::vector<std::vector<int>> &graph, const int &startNode, const int &repetitionsCount) {
    const auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < repetitionsCount; ++i) {
        findShortPath(graph, startNode);
    }
    const auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    return elapsed.count();
}

std::vector<std::vector<int>> shortPathDefault() {
    return generateLargeGraph();
}

std::vector<std::vector<int>> shortPathCustom() {
    int n;
    std::cout << "Enter the number of nodes in the graph: ";
    std::cin >> n;

    std::vector<std::vector<int>> graph(n, std::vector<int>(n, 0));
    std::cout << "Enter the adjacency matrix (0 if no edge):\n";
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            std::cin >> graph[i][j];
        }
    }

    return graph;
}
