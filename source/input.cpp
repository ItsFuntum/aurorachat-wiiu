#include "input.h"
#include "chat.h"
#include "theme.h"

std::string scene = "selection_menu";
std::string textSendType = "";

int rulesPage = 0;

void handle_button_down(const SDL_ControllerButtonEvent& e)
{
    if (textSendType.empty()) {
        if (scene == "chat") {
            if (e.button == SDL_CONTROLLER_BUTTON_A) {
                textSendType = "message";
                SDL_WiiUSetSWKBDHintText("Say something...");
                SDL_StartTextInput();
            }
            else if (e.button == SDL_CONTROLLER_BUTTON_Y) {
                if (rulesPage < 3) {
                    rulesPage += 1;
                }
                else {
                    rulesPage = 0;
                }
            }
        }
        else if (scene == "invalid_credentials") {
            if (e.button == SDL_CONTROLLER_BUTTON_B) {
                scene = "selection_menu";
            }
        }

        if (e.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER) {
            currentTheme--;
            if (currentTheme < 0) currentTheme = THEME_COUNT - 1;
            ApplyTheme(currentTheme);
        }
        else if (e.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) {
            currentTheme++;
            if (currentTheme >= THEME_COUNT) currentTheme = 0;
            ApplyTheme(currentTheme);
        }
        else if (e.button == SDL_CONTROLLER_BUTTON_X) {
            // Swap TV and DRC colors
            SDL_Color tempBg = tvBackgroundColor;
            SDL_Color tempText = tvTextColor;

            tvBackgroundColor = drcBackgroundColor;
            tvTextColor = drcTextColor;

            drcBackgroundColor = tempBg;
            drcTextColor = tempText;

            if (tvRenderer)
                RebuildChatTextures(tvRenderer, fontSize, tvTextColor, maxWidth);
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
