cmake_minimum_required(VERSION 3.10)
project(aurorachat-wiiu C CXX)

# Collect ALL source files
file(GLOB SOURCES
    source/*.cpp
)

# Create executable
add_executable(aurorachat-wiiu ${SOURCES})

# Include paths
target_include_directories(aurorachat-wiiu PRIVATE
    ${DEVKITPRO}/portlibs/wiiu/include
    ${CMAKE_SOURCE_DIR}/include
)

# RomFS
include("${DEVKITPRO}/portlibs/wiiu/share/romfs-wiiu.cmake" REQUIRED)
romfs_add(aurorachat-wiiu "${CMAKE_SOURCE_DIR}/romfs")

# Link libraries
target_link_libraries(aurorachat-wiiu
    SDL2
    SDL2_ttf
    SDL2_image
    webp
    freetype
    harfbuzz
    png
    z
    bz2
    brotlidec
    brotlienc
    brotlicommon
)

# Create RPX
wut_create_rpx(aurorachat-wiiu)

# Create WUHB
add_custom_command(TARGET aurorachat-wiiu POST_BUILD
    COMMAND wuhbtool
        ${CMAKE_CURRENT_BINARY_DIR}/aurorachat-wiiu.rpx
        ${CMAKE_CURRENT_BINARY_DIR}/aurorachat-wiiu.wuhb
        --name "aurorachat for Wii U"
        --short-name "aurorachat"
        --author "Funtum"
        --icon ${CMAKE_SOURCE_DIR}/meta/icon.png
        --tv-image ${CMAKE_SOURCE_DIR}/meta/banner.png
        --drc-image ${CMAKE_SOURCE_DIR}/meta/banner.png
    COMMENT "Packaging aurorachat-wiiu.wuhb..."
)
