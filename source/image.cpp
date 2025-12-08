#include "image.h"
#include "chat.h"
#include <SDL2/SDL_image.h>

void DrawImage(SDL_Renderer* renderer, int x, int y, const char* file) {
    SDL_Texture* texture = IMG_LoadTexture(renderer, file);
    if (!texture) {
        AddChatLine(std::string("Failed to load image: ") + IMG_GetError());
        return;
    }

    int w, h;
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
    SDL_Rect dst = { x, y, w, h };
    SDL_RenderCopy(renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}
