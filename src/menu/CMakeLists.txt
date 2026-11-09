# eracer-menu: native replacement for the Delphi menu (Menu/GameMenu.dat).
#   engine/   Delphi-form menu engine, copied from Vakantie Racer and extended
#             here (RCS archives, TGA, TGDI components, Forms.ini layout)
#   launcher/ RED-engine race handshake (game.ini + running the race)
#   er/       Europe Racer flow (GameMenu.ini flow matrix), data and screens
find_package(SDL3 REQUIRED CONFIG)
find_package(Freetype REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MENU_FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libavutil libswscale libswresample)

file(GLOB ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp)
add_library(davi_menu STATIC ${ENGINE_SOURCES} launcher/race_launcher.cpp)
target_include_directories(davi_menu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(davi_menu PUBLIC SDL3::SDL3 Freetype::Freetype ZLIB::ZLIB PkgConfig::MENU_FFMPEG)

file(GLOB ER_MENU_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/er/*.cpp)
add_executable(eracer-menu ${ER_MENU_SOURCES})
target_link_libraries(eracer-menu PRIVATE davi_menu)
set_target_properties(eracer-menu PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
