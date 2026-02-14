#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>

struct ChatLine {
    std::string rawText;
    SDL_Texture* texture;
    int height;
};

extern std::vector<ChatLine> chatLines;
extern int chatPosY;

void AddChatLine(SDL_Renderer* renderer, const std::string& text, int fontSize, SDL_Color color, int maxWidth);

void DrawChatBuffer(SDL_Renderer* renderer, int x, int y);

void FreeChatTextures();

void RebuildChatTextures(SDL_Renderer* renderer, int fontSize, SDL_Color color, int maxWidth);
