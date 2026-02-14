#pragma once
#include <SDL2/SDL.h>

extern SDL_Color buttonTextColor;

void DrawButtonWithText(
    SDL_Renderer* r,
    SDL_Texture* button,
    SDL_Rect rect,
    const char* label,
    int size
);
