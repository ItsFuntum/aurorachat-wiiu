cmake_minimum_required(VERSION 3.10)
project(aurorachat-wiiu CXX C)

# Tell CMake where to find SDL2
include_directories(aurorachat-wiiu PRIVATE
   ${DEVKITPRO}/portlibs/wiiu/include
)

add_executable(aurorachat-wiiu
   source/main.cpp)

include("${DEVKITPRO}/portlibs/wiiu/share/romfs-wiiu.cmake" REQUIRED)
romfs_add(aurorachat-wiiu "${CMAKE_SOURCE_DIR}/romfs")

target_link_libraries(aurorachat-wiiu
   SDL2
   SDL2_ttf
   freetype
   harfbuzz)

wut_create_rpx(aurorachat-wiiu)