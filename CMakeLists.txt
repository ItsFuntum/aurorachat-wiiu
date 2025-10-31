cmake_minimum_required(VERSION 3.10)
project(aurorachat-wiiu CXX C)

include_directories(
   ${CMAKE_SOURCE_DIR}/SDL/include
   ${CMAKE_SOURCE_DIR}/SDL/src
   ${CMAKE_SOURCE_DIR}/SDL/src/include
   ${DEVKITPRO}/portlibs/wiiu/include)
link_directories(
   ${CMAKE_SOURCE_DIR}/SDL/src)

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

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/aurorachat-wiiu.rpx"
   DESTINATION "${CMAKE_INSTALL_PREFIX}")