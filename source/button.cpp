#include "button.h"
#include "font.h"

SDL_Color buttonTextColor = { 0, 0, 0, 100 };

void DrawButtonWithText(
    SDL_Renderer* r,
    SDL_Texture* button,
    SDL_Rect rect,
    const char* label,
    int size) {
    TTF_Font* font = GetFont(size);
    if (!font) return;

    SDL_RenderCopy(r, button, NULL, &rect);

    int tw, th;
    TTF_SizeUTF8(font, label, &tw, &th);

    int tx = rect.x + (rect.w - tw) / 2;
    int ty = rect.y + (rect.h - th) / 2;

    DrawText(r, label, tx, ty, size, buttonTextColor);
}
