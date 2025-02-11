add_executable(shell directory.cpp directory.h execute_command.cpp execute_command.h shell_main.cpp)
target_include_directories(shell PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})