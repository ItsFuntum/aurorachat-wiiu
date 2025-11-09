# [aurorachat](https://github.com/mii-man/aurorachat) for Wii U
A chatting application originally for the Nintendo 3DS and 2DS line of systems, now on Wii U.

## Features

- [X] SDL2 Rendering
- [X] Wii U Keyboard support
- [ ] Themes
- [ ] Auroraccounts integration (can't be done yet)
- [ ] Scrollable chat
- [ ] Sounds
- [ ] Background music (we need Nintendo vibes)
- [ ] Dual-screen rendering (I haven't figured this out yet, it may not even be possible)

## Goal

The goal with this project is to allow Wii U users to chat on aurorachat. We also want to replicate the Nintendo 3DS/2DS client as close as possible in terms of visuals; you can think of it as a port of the original app.

# Building

Before building make sure you have the following dependencies installed:
- wut
- [dkosmari's SDL fork](https://github.com/dkosmari/SDL/tree/wiiu-swkb)
- SDL2_ttf
- SDL2_image
- libromfs-wiiu

Once the environment is setup:
```
git clone https://github.com/ItsFuntum/aurorachat-wiiu.git
cd aurorachat-wiiu
mkdir build && cd build
/opt/devkitpro/portlibs/wiiu/bin/powerpc-eabi-cmake ../
make
```
