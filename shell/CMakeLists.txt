# Установка минимальной версии CMake
cmake_minimum_required(VERSION 3.10)

# Имя проекта
project(MyProject)

# Отключение оптимизации для всех целей
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0") # Отключение оптимизации
set(CMAKE_CXX_FLAGS_DEBUG "-O0")             # Для режима Debug
set(CMAKE_CXX_FLAGS_RELEASE "-O0")           # Для режима Release

# Указание языка C++ стандарт
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

# Добавление поддиректорий src
add_subdirectory(src)