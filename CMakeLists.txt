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
   SDL2_image
   freetype
   harfbuzz)

wut_create_rpx(aurorachat-wiiu)

add_custom_command(TARGET aurorachat-wiiu POST_BUILD
    COMMAND wuhbtool
        ${CMAKE_CURRENT_BINARY_DIR}/aurorachat-wiiu.rpx
        ${CMAKE_CURRENT_BINARY_DIR}/aurorachat-wiiu.wuhb
        --name="aurorachat-wiiu"
        --short-name="aurorachat"
        --author="Funtum"
        --icon=${CMAKE_SOURCE_DIR}/aurorachat-wiiu-icon.png
    COMMENT "Packaging aurorachat-wiiu.wuhb..."
)