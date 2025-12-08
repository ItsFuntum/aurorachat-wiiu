#pragma once
#include <deque>
#include <string>
#include <SDL2/SDL.h>

extern int chatPosY;

void AddChatLine(const std::string& msg);
void DrawChatBuffer(SDL_Renderer* renderer, int startX, int startY, int lineHeight, SDL_Color color);
