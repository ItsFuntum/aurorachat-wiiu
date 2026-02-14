#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <map>

TTF_Font* GetFont(int size);
void FreeFonts();
void DrawText(SDL_Renderer* renderer, const char* text, int x, int y, int size, SDL_Color color);

SDL_Texture* DrawTextToTexture(SDL_Renderer* renderer, const char* text, int size, SDL_Color color, int maxWidth);
