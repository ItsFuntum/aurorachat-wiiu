cmake_minimum_required(VERSION 3.10)
project(aurorachat-wiiu CXX C)

set(CMAKE_TOOLCHAIN_FILE "$ENV{DEVKITPRO}/wut/share/wut-toolchain.cmake")
set(CMAKE_BUILD_TYPE Release)

# Tell CMake where to find SDL2
include_directories(aurorachat-wiiu PRIVATE
   ${CMAKE_SOURCE_DIR}/libs/sdl2/include/SDL2
)

link_directories(aurorachat-wiiu PRIVATE
   ${CMAKE_SOURCE_DIR}/libs/sdl2/lib
)

add_executable(aurorachat-wiiu
   source/main.cpp)

include("$ENV{DEVKITPRO}/portlibs/wiiu/share/romfs-wiiu.cmake" REQUIRED)

romfs_add(aurorachat-wiiu "${CMAKE_SOURCE_DIR}/romfs")

target_link_libraries(aurorachat-wiiu
   ${CMAKE_SOURCE_DIR}/libs/sdl2/lib/libSDL2.a
   SDL2
   SDL2_ttf
   freetype
   harfbuzz)

wut_create_rpx(aurorachat-wiiu)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/aurorachat-wiiu.rpx"
   DESTINATION "${CMAKE_INSTALL_PREFIX}")