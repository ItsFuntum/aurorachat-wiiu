#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>
#include <string>

extern std::string scene;
extern std::string username;
extern std::string textBuffer;
extern std::string textSendType;

void handle_event(const SDL_Event& event);
void handle_button_down(const SDL_ControllerButtonEvent& e);
