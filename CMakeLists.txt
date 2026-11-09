cmake_minimum_required(VERSION 3.25)
project(eracer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The game itself (spel.exe lifted with SwARc) comes later, see CLAUDE.md.

# Native menu (replaces Menu/GameMenu.dat), see src/menu/.
add_subdirectory(src/menu)
