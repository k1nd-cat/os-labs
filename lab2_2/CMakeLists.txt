cmake_minimum_required(VERSION 3.15)
project(NRU_Cache_Benchmark CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0A00)
    set(PLATFORM_LIBS Kernel32.lib)
endif()

add_library(nru_cache SHARED
        include/file_operations.h
        src/file_operations.cpp
        include/nru_cache.h
        src/nru_cache.cpp
)

target_include_directories(nru_cache PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(nru_cache_benchmark
        app/main.cpp
)

target_link_libraries(nru_cache_benchmark
        PRIVATE nru_cache ${PLATFORM_LIBS}
)

set_target_properties(nru_cache_benchmark PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)