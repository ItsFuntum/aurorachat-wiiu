#include "font.h"

static std::map<int, TTF_Font*> g_FontCache;
static const char* g_FontPath = "romfs:/res/FOT-RodinNTLG Pro DB.otf";

TTF_Font* GetFont(int size) {
    if (g_FontCache.count(size))
        return g_FontCache[size];

    TTF_Font* font = TTF_OpenFont(g_FontPath, size);
    if (!font) return nullptr;

    g_FontCache[size] = font;
    return font;
}

void FreeFonts() {
    for (auto& f : g_FontCache)
        TTF_CloseFont(f.second);
    g_FontCache.clear();
}

void DrawText(SDL_Renderer* renderer, const char* text, int x, int y, int size, SDL_Color color) {
    TTF_Font* font = GetFont(size);
    if (!font) return;

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    int w, h;
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);

    SDL_Rect dst = { x, y, w, h };
    SDL_RenderCopy(renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

SDL_Texture* DrawTextToTexture(SDL_Renderer* renderer, const char* text, int size, SDL_Color color, int maxWidth)
{
    TTF_Font* font = GetFont(size);
    if (!font) return nullptr;

    SDL_Surface* surface =
        TTF_RenderUTF8_Blended_Wrapped(font, text, color, maxWidth);

    if (!surface) return nullptr;

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(renderer, surface);

    SDL_FreeSurface(surface);

    return texture;
}
