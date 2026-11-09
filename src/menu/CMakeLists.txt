# eracer-menu: native replacement for the Delphi menu (Menu/GameMenu.dat).
#   engine    shared with Vakantie Racer and A2 Racer IV: ../Davilex-RED/menu/engine
#   launcher/ RED-engine race handshake (game.ini + running the race)
#   er/       Europe Racer flow (GameMenu.ini flow matrix), data and screens
set(DAVILEX_RED_DIR ${CMAKE_SOURCE_DIR}/../Davilex-RED CACHE PATH "Shared Davilex RED code")
include(${DAVILEX_RED_DIR}/menu/davi_menu.cmake)

add_library(davi_menu STATIC launcher/race_launcher.cpp)
target_include_directories(davi_menu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(davi_menu PUBLIC davi_menu_engine)

file(GLOB ER_MENU_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/er/*.cpp)
add_executable(eracer-menu ${ER_MENU_SOURCES})
target_link_libraries(eracer-menu PRIVATE davi_menu)
set_target_properties(eracer-menu PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
