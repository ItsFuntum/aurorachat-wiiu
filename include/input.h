#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>
#include <string>

extern std::string username;
extern std::string scene;
extern std::string textSendType;

extern int rulesPage;

extern bool isThemeReversed;

void handle_button_down(const SDL_ControllerButtonEvent& e);
void handle_event(const SDL_Event& event);

bool PointInRect(int x, int y, const SDL_Rect& r);
