#include "chat.h"
#include "font.h"

static std::deque<std::string> g_ChatBuffer;
int chatPosY = 0;

void AddChatLine(const std::string& msg) {
    g_ChatBuffer.push_back(msg);
}

void DrawChatBuffer(SDL_Renderer* renderer, int startX, int startY, int lineHeight, SDL_Color color) {
    int y = startY + chatPosY;

    for (const auto& line : g_ChatBuffer) {
        DrawText(renderer, line.c_str(), startX, y, 24, color);
        y += lineHeight;
    }
}
