#include <iostream>
#include <string>

#include "directory.h"
#include "execute_command.h"

int main() {
    setlocale(LC_ALL, "");
    std::wcout << L"Лабораторная работа №1" << std::endl;
    std::wcout << L"(c) Трошкин Александр (Troshkin Aleksandr). Ни одного права не защищено." << std::endl << std::endl;

    auto directory = new Directory();
    std::string input;

    while (true) {
        std::string currentDirectory = directory->getCurrentDirectory();
        std::cout << currentDirectory << ">" << std::flush;
        std::getline(std::cin, input);

        if (input == "exit") break;

        execute_command(input, *directory);
    }

    return 0;
}
