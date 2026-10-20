cmake_minimum_required(VERSION 3.20)
project(chip8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # helps VS Code understand the project

# The emulator core has no graphics dependency, so tests can use it directly.
add_library(chip8_core src/chip8.cpp)
target_include_directories(chip8_core PUBLIC src)
target_compile_options(chip8_core PRIVATE -Wall -Wextra -Wpedantic)

# The desktop app: core + SDL2 window and keyboard.
find_package(SDL2 REQUIRED CONFIG)
add_executable(chip8 src/main.cpp)
target_link_libraries(chip8 PRIVATE chip8_core SDL2::SDL2)
target_compile_options(chip8 PRIVATE -Wall -Wextra -Wpedantic)
