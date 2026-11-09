# eracer-menu: native replacement for the Delphi menu (Menu/GameMenu.dat).
#   engine    shared with Vakantie Racer and A2 Racer IV: ../Davilex-RED/menu/engine
#   launcher  shared too: ../Davilex-RED/menu/launcher (Install paths, running the race)
#   er/       Europe Racer flow (GameMenu.ini flow matrix), data and screens
set(DAVILEX_RED_DIR ${CMAKE_SOURCE_DIR}/../Davilex-RED CACHE PATH "Shared Davilex RED code")
include(${DAVILEX_RED_DIR}/menu/davi_menu.cmake)


file(GLOB ER_MENU_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/er/*.cpp)
add_executable(eracer-menu ${ER_MENU_SOURCES})
target_include_directories(eracer-menu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(eracer-menu PRIVATE davi_menu_engine davi_menu_launcher)
set_target_properties(eracer-menu PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
