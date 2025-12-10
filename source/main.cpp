#include <whb/proc.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <romfs-wiiu.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_syswm.h>

#include "theme.h"
#include "font.h"
#include "chat.h"
#include "image.h"
#include "net.h"
#include "input.h"

// -----------------------
// Main
// -----------------------
int main(int argc, char **argv)
{
    WHBProcInit();
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    romfsInit();
    TTF_Init();
    IMG_Init(IMG_INIT_PNG);

    int sock = ConnectToServer();

    char input[512] = "";
    
    // Initialize Wii U video subsystem
    SDL_Window *tvWindow = NULL;
    SDL_Window *drcWindow = NULL;
    SDL_Renderer *tvRenderer = NULL;
    SDL_Renderer *drcRenderer = NULL;

    // Set vsync hint before creating windows
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    // TV Window (primary display)
    tvWindow = SDL_CreateWindow("TV", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        1280, 720,  // Use 720p resolution
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_WIIU_TV_ONLY);
    if (tvWindow) {
        tvRenderer = SDL_CreateRenderer(tvWindow, 0,  // Use first display driver
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    }

    // GamePad Window
    drcWindow = SDL_CreateWindow("DRC",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        854, 480,  // Native GamePad resolution
        SDL_WINDOW_WIIU_GAMEPAD_ONLY | SDL_WINDOW_WIIU_PREVENT_SWAP);
    if (drcWindow) {
        drcRenderer = SDL_CreateRenderer(drcWindow, 1,  // Use second display driver
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    }

    SDL_Color black = {0, 0, 0, 255};

    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

    AddChatLine("-chat-");

    SDL_Texture* logoTexture = LoadImage(tvRenderer, "romfs:/res/logo.png");
    SDL_Rect logoRect = {1350, 10, 0, 0};
    SDL_QueryTexture(logoTexture, NULL, NULL, &logoRect.w, &logoRect.h);

    Uint32 lastTicks = 0;
    const int AXIS_DEADZONE = 8000;  // deadzone for joystick
    const float MAX_SPEED = 300.0f;  // pixels per second when stick is fully pushed

    SDL_Event event;
    SDL_GameController* gController = nullptr;

    if (SDL_NumJoysticks() > 0) {
        if (SDL_IsGameController(0)) {
            gController = SDL_GameControllerOpen(0);
        }
    }

    int Keyboard_Event;
    SDL_WiiUSysWMEventType Keyboard_Ok = SDL_WIIU_SYSWM_SWKBD_OK_FINISH_EVENT;
    SDL_WiiUSysWMEventType Keyboard_Cancel = SDL_WIIU_SYSWM_SWKBD_CANCEL_EVENT;

    lastTicks = SDL_GetTicks();
    while (WHBProcIsRunning()) {
        // Continuous axis polling to move chatPosY while held
        Uint32 now = SDL_GetTicks();
        float deltaSec = (now - lastTicks) / 1000.0f;
        lastTicks = now;
        if (gController) {
            Sint16 axisY = SDL_GameControllerGetAxis(gController, SDL_CONTROLLER_AXIS_LEFTY);
        
            if (axisY > AXIS_DEADZONE || axisY < -AXIS_DEADZONE) {
                // Normalize axis value to -1.0 .. 1.0
                float norm = axisY / 32767.0f;  // note: axisY is signed
                // Multiply by speed and frame delta to get pixel movement
                float move = norm * MAX_SPEED * deltaSec;
                chatPosY -= (int)move;
            }
        }

        while (SDL_PollEvent(&event)) {
            handle_event(event);

            if (event.type == SDL_TEXTINPUT)
                textBuffer += event.text.text;

            if (event.type == SDL_SYSWMEVENT) {
                Keyboard_Event = event.syswm.msg->msg.wiiu.event;
                if (Keyboard_Event == Keyboard_Ok || Keyboard_Event == Keyboard_Cancel) {
                    if (Keyboard_Event == Keyboard_Ok) {
                        if (textSendType == "message" && !textBuffer.empty()) {
                            strncpy(input, textBuffer.c_str(), sizeof(input) - 1);
                            input[sizeof(input) - 1] = '\0';
                            send_chat_line(&sock, username.c_str(), input);
                        } else if (textSendType == "username") {
                            username = textBuffer;
                        }
                    }
                    textBuffer.clear();
                    textSendType.clear();
                    SDL_StopTextInput();
                }
            }
        }

        // Handle incoming messages
        TryReceive(&sock);

        // Render TV Screen
        if (tvRenderer) {
            SDL_SetRenderDrawColor(tvRenderer, current.backgroundColor.r, current.backgroundColor.g, current.backgroundColor.b, current.backgroundColor.a);
            SDL_RenderClear(tvRenderer);

            if (scene == "main") {
                // TV content
                DrawText(tvRenderer, "aurorachat", 1300, 10, 96, current.textColor);
                DrawText(tvRenderer, "v0.0.4", 1700, 120, 64, current.textColor);
                DrawText(tvRenderer, (current.name).c_str(), 820, 0, 32, current.textColor);
                DrawText(tvRenderer, "A: Change Username", 0, 20, 64, current.textColor);
                DrawText(tvRenderer, "B: Send Message", 0, 110, 64, current.textColor);
                DrawText(tvRenderer, "L: Rules", 0, 200, 64, current.textColor);
                DrawText(tvRenderer, "D-PAD: Change Theme", 0, 290, 64, current.textColor);
                DrawText(tvRenderer, ("Username: " + username).c_str(), 0, 900, 96, current.textColor);

                SDL_RenderCopy(tvRenderer, logoTexture, NULL, &logoRect);
            }
            else if (scene == "rules") {
                DrawText(tvRenderer, "Rule 2: No Swearing", 0, 380, 64, current.textColor);
                DrawText(tvRenderer, "(Press X to Go Back)", 0, 20, 64, current.textColor);
                DrawText(tvRenderer, "Rule 1: No Spamming", 0, 200, 64, current.textColor);
                DrawText(tvRenderer, "Rule 3: No Impersonating", 0, 560, 64, current.textColor);
                DrawText(tvRenderer, "Rule 4: No Politics", 0, 740, 64, current.textColor);
                DrawText(tvRenderer, "Breaking rules may result in a ban", 0, 920, 64, current.textColor);
            }
            SDL_RenderPresent(tvRenderer);
        }

        // Render DRC (GamePad) Screen
        if (drcRenderer) {
            SDL_SetRenderDrawColor(drcRenderer, current.backgroundColor.r, current.backgroundColor.g, current.backgroundColor.b, current.backgroundColor.a);
            SDL_RenderClear(drcRenderer);

            DrawChatBuffer(drcRenderer, 0, 40, 40, current.textColor);
            SDL_RenderPresent(drcRenderer);
        }
    }

    if (sock >= 0) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }

    if (gController)
        SDL_GameControllerClose(gController);

    if (drcRenderer)
        SDL_DestroyRenderer(drcRenderer);
    if (drcWindow)
        SDL_DestroyWindow(drcWindow);
    if (tvRenderer)
        SDL_DestroyRenderer(tvRenderer);
    if (tvWindow)
        SDL_DestroyWindow(tvWindow);

    IMG_Quit();
    FreeFonts();
    TTF_Quit();
    romfsExit();
    SDL_Quit();
    WHBProcShutdown();
    return 0;
}
