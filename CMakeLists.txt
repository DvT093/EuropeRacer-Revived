cmake_minimum_required(VERSION 3.25)
project(eracer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_LINK_DEPENDS_NO_SHARED ON)

# SwARc (lifter, x87 model, runtime, Win32 HLE): Europe Racer's own worktree
# ../SwARc-er (branch er, from A2's a2r4), never another game's.
set(SWARC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../SwARc-er" CACHE PATH "SwARc source tree")

if(EXISTS "${SWARC_DIR}/CMakeLists.txt")
  enable_testing()
  add_subdirectory("${SWARC_DIR}" swarc)

  # The NoCD spel.exe lifted to C++ in generated/ (./lift.sh, see CLAUDE.md).
  recomp_add_lifted(eracer_lifted "${CMAKE_CURRENT_SOURCE_DIR}/generated")
  if(TARGET eracer_lifted)
    # src/game: hand-written replacements of lifted functions
    # (lifter.manifest "replace"), one file per original source file.
    file(GLOB ERACER_GAME_SOURCES CONFIGURE_DEPENDS src/game/*.cpp)
    add_executable(eracer src/main.cpp ${ERACER_GAME_SOURCES} $<TARGET_OBJECTS:eracer_lifted>)
    target_include_directories(eracer PRIVATE src)
    target_link_libraries(eracer PRIVATE recomp_win32 recomp_d3d7 recomp_dinput8 recomp_dsound recomp_mss32)
    set_target_properties(eracer PROPERTIES ENABLE_EXPORTS ON)
  endif()
endif()

# Native menu (replaces Menu/GameMenu.dat), see src/menu/.
add_subdirectory(src/menu)
