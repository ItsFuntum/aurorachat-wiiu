> [!IMPORTANT]
> aurorachat for Wii U is currently incompatible with the latest version of aurorachat, please wait for an update.

# [aurorachat](https://github.com/mii-man/aurorachat) for Wii U
A chatting application originally for the Nintendo 3DS and 2DS line of systems, now on Wii U.

## Goal

The goal with this project is to allow Wii U users to chat on aurorachat. We also want to replicate the Nintendo 3DS/2DS client as close as possible in terms of visuals; you can think of it as a port of the original app.

# Building

Before building make sure you have the following dependencies installed:
- wut
- SDL
- SDL2_ttf
- SDL2_image
- libromfs-wiiu

Once the environment is setup:
```
git clone https://github.com/ItsFuntum/aurorachat-wiiu.git
cd aurorachat-wiiu
mkdir build && cd build
/opt/devkitpro/portlibs/wiiu/bin/powerpc-eabi-cmake ../ -DCMAKE_POLICY_VERSION_MINIMUM=3.5
make
```
