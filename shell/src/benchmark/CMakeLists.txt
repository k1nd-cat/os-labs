# bench1
add_executable(bench1 bench1.cpp bench1_main.cpp bench1.h)
target_include_directories(bench1 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# bench2
add_executable(bench2 bench2.cpp bench2_main.cpp bench2.h)
target_include_directories(bench2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# multi_bench
add_executable(multi_bench multi_bench.cpp bench1.cpp bench2.cpp bench1.h bench2.h)
target_include_directories(multi_bench PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})