#ifndef SHELL_BENCH2_H
#define SHELL_BENCH2_H

#include <vector>

double shortPath(const int& repetitionsCount);

double shortPathDefault(const int &repetitionsCount);

std::vector<std::vector<int>> shortPathCustom();

std::vector<std::vector<int>> shortPathDefault();

double measureShortPathTime(const std::vector<std::vector<int>> &graph, const int &startNode, const int &repetitionsCount);

#endif //SHELL_BENCH2_H
