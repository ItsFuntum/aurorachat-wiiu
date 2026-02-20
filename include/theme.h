#pragma once
#include <SDL2/SDL.h>

// These are defined in main.cpp
extern SDL_Renderer* tvRenderer;
extern int fontSize;
extern int maxWidth;
extern SDL_Color tvBackgroundColor;
extern SDL_Color tvTextColor;
extern SDL_Color drcBackgroundColor;
extern SDL_Color drcTextColor;
extern SDL_Color logoColor;

struct Theme {
    SDL_Color tvText;
    SDL_Color tvBackground;

    SDL_Color drcText;
    SDL_Color drcBackground;

    SDL_Color logoColor;
    const char* name;
};

extern Theme themes[];
extern int currentTheme;
extern const int THEME_COUNT;

void ApplyTheme(int index);
