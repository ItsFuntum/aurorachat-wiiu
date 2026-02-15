#include "input.h"
#include "chat.h"

std::string scene = "selection_menu";
std::string textSendType = "";

SDL_Color textColor = {0, 0, 0, 255};
SDL_Color textColor_lowOpacity = {0, 0, 0, 120};
SDL_Color themeColor = {255, 255, 255, 255};

bool darkMode = false;

void handle_button_down(const SDL_ControllerButtonEvent& e)
{
    if (textSendType.empty()) {
        if (e.button == SDL_CONTROLLER_BUTTON_X) {
            darkMode = !darkMode;
            
            textColor = darkMode ? SDL_Color{255,255,255,255} : SDL_Color{0,0,0,255};
            textColor_lowOpacity = darkMode ? SDL_Color{255,255,255,120} : SDL_Color{0,0,0,120};
            themeColor = darkMode ? SDL_Color{0,0,0,255} : SDL_Color{255,255,255,255};
        }

        if (scene == "chat") {
            if (e.button == SDL_CONTROLLER_BUTTON_A) {
                textSendType = "message";
                SDL_WiiUSetSWKBDHintText("Say something...");
                SDL_StartTextInput();
            }
        }
        else if (scene == "invalid_credentials") {
            if (e.button == SDL_CONTROLLER_BUTTON_B) {
                scene = "selection_menu";
            }
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

// Touch Input
bool PointInRect(int x, int y, const SDL_Rect& r) {
    return (x >= r.x &&
            x <  r.x + r.w &&
            y >= r.y &&
            y <  r.y + r.h);
}
