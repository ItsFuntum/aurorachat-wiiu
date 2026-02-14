#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

SDL_Texture* LoadImage(SDL_Renderer* renderer, const char* path);
