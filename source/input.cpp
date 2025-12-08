#include "theme.h"
#include "input.h"

std::string scene = "main";
std::string username = "";
std::string textBuffer = "";
std::string textSendType = "";

void handle_button_down(const SDL_ControllerButtonEvent& e)
{
    if (textSendType.empty()) {
        if (scene == "main") {
            if (e.button == SDL_CONTROLLER_BUTTON_A) {
                textSendType = "username";
                SDL_WiiUSetSWKBDInitialText(username.c_str());
                SDL_StartTextInput();
            }
            else if (e.button == SDL_CONTROLLER_BUTTON_B) {
                textSendType = "message";
                SDL_StartTextInput();
            }
            else if (e.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER) {
                scene = "rules";
            }
            else if (e.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
                currentTheme++;
                if (currentTheme > Themes.size()) currentTheme = 1;
                current = Themes[currentTheme];
            }
            else if (e.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
                currentTheme--;
                if (currentTheme < 1) currentTheme = Themes.size();
                current = Themes[currentTheme];
            }
        }
        else if (e.button == SDL_CONTROLLER_BUTTON_X) {
            scene = "main";
        }
    }
}

void handle_event(const SDL_Event& event)
{
    switch (event.type) {
        case SDL_CONTROLLERDEVICEADDED:
            SDL_GameControllerOpen(event.cdevice.which);
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            if (auto ctrlr = SDL_GameControllerFromInstanceID(event.cdevice.which))
                SDL_GameControllerClose(ctrlr);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            handle_button_down(event.cbutton);
            break;
    }
}