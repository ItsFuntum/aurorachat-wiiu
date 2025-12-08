#pragma once
#include <SDL2/SDL.h>
#include <map>
#include <string>

struct Theme {
    SDL_Color backgroundColor;
    SDL_Color textColor;
    std::string name;
};

extern std::map<int, Theme> Themes;
extern Theme current;
extern int currentTheme;
