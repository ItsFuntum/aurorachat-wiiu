#include "image.h"
#include "chat.h"

SDL_Texture* LoadImage(SDL_Renderer* renderer, const char* path) {
    SDL_Surface* surface = IMG_Load(path);  // Load PNG/JPG/etc
    if (!surface) {
        SDL_Log("Failed to load image %s: %s", path, IMG_GetError());
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture) {
        SDL_Log("Failed to create texture from %s: %s", path, SDL_GetError());
        return nullptr;
    }

    return texture;
}
