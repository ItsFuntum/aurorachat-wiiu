#include <whb/proc.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <romfs-wiiu.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_syswm.h>

#include "font.h"
#include "chat.h"
#include "image.h"
#include "net.h"
#include "input.h"
#include "button.h"

// -----------------------
// Main
// -----------------------
int main(int argc, char **argv)
{
    WHBProcInit();
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO);
    romfsInit();
    TTF_Init();
    IMG_Init(IMG_INIT_PNG);

    // Keyboard Text Input Buffer
    std::string textBuffer = "";

    std::string username = "";
    std::string password = "";

    bool showpassword = false;

    int sock = ConnectToTCPServer();

    char input[512] = "";

    // Initialize audio to stop loading screen music from playing
    SDL_AudioSpec want{}, have{};
    want.freq = 48000;
    want.format = AUDIO_S16;
    want.channels = 2;
    want.samples = 4096;
    want.callback = nullptr;

    SDL_OpenAudio(&want, &have);
    SDL_PauseAudio(0);
    
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

    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

    AddChatLine("-chat-");

    // Button Texture
    SDL_Texture* buttonTexture = IMG_LoadTexture(drcRenderer, "romfs:/res/largebutton.png");
    int bw, bh;
    SDL_QueryTexture(buttonTexture, NULL, NULL, &bw, &bh);

    // Buttons
    SDL_Rect button_middle_top = { 0, 50, 0, 0 };
    button_middle_top.w = bw;
    button_middle_top.h = bh;
    button_middle_top.x = (854 - button_middle_top.w) / 2;

    SDL_Rect button_middle_bottom = { 0, 0, 0, 0 };
    button_middle_bottom.w = bw;
    button_middle_bottom.h = bh;
    button_middle_bottom.x = (854 - button_middle_bottom.w) / 2;
    button_middle_bottom.y = (480 - 50) / 2;

    SDL_Rect button_right_bottom = { 854, 480, 0, 0 };
    button_right_bottom.w = bw;
    button_right_bottom.h = bh;
    button_right_bottom.x = 854 - button_right_bottom.w;
    button_right_bottom.y = 480 - button_right_bottom.h;

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

            if (event.type == SDL_MOUSEBUTTONDOWN &&
                event.button.button == SDL_BUTTON_LEFT) {

                int mx = event.button.x;
                int my = event.button.y;

                if (PointInRect(mx, my, button_middle_top )) {
                    if (scene == "selection_menu") scene = "sign_up";
                    else if (scene == "sign_up" || scene == "sign_in") {
                        textSendType = "username";
                        SDL_WiiUSetSWKBDInitialText(username.c_str());
                        SDL_WiiUSetSWKBDHintText("Enter a username...");
                        SDL_StartTextInput();
                    }
                    else if (scene == "sign_up_confirm") {
                        make_account(username.c_str(), password.c_str());
                        scene = "selection_menu";
                    }
                    else if (scene == "sign_in_confirm") {
                        login_account(username.c_str(), password.c_str());
                        scene = "chat";
                    }
                }
                else if (PointInRect(mx, my, button_middle_bottom)) {
                    if (scene == "selection_menu") scene = "sign_in";
                    else if (scene == "sign_up" || scene == "sign_in") {
                        textSendType = "password";
                        SDL_WiiUSetSWKBDInitialText(password.c_str());
                        SDL_WiiUSetSWKBDHintText("Enter a password...");
                        if (!showpassword) SDL_WiiUSetSWKBDPasswordMode(SDL_WIIU_SWKBD_PASSWORD_MODE_HIDE);
                        else SDL_WiiUSetSWKBDPasswordMode(SDL_WIIU_SWKBD_PASSWORD_MODE_SHOW);
                        SDL_StartTextInput();
                    }
                    else if (scene == "sign_up_confirm" || scene == "sign_in_confirm") {
                        showpassword = !showpassword;
                    }
                }
                else if (PointInRect(mx, my, button_right_bottom)) {
                    if (scene == "sign_up") scene = "sign_up_confirm";
                    else if (scene == "sign_in") scene = "sign_in_confirm";
                    else if (scene == "chat") {
                        textSendType = "message";
                        SDL_WiiUSetSWKBDHintText("Say something...");
                        SDL_StartTextInput();
                    }
                }
            }

            if (event.type == SDL_TEXTINPUT)
                textBuffer += event.text.text;

            if (event.type == SDL_SYSWMEVENT) {
                Keyboard_Event = event.syswm.msg->msg.wiiu.event;
                if (Keyboard_Event == Keyboard_Ok || Keyboard_Event == Keyboard_Cancel) {
                    if (Keyboard_Event == Keyboard_Ok) {
                        if (textSendType == "message" && !textBuffer.empty()) {
                            strncpy(input, textBuffer.c_str(), sizeof(input) - 1);
                            input[sizeof(input) - 1] = '\0';
                            send_chat(username.c_str(), password.c_str(), input);
                        }
                        else if (textSendType == "username") {
                            username = textBuffer;
                        }
                        else if (textSendType == "password") {
                            password = textBuffer;
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
            SDL_SetRenderDrawColor(tvRenderer, themeColor.r, themeColor.g, themeColor.b, themeColor.a);
            SDL_RenderClear(tvRenderer);

            DrawText(tvRenderer, "Aurorachat", 1500, 20, 64, { 0, 0, 100, 200 });
            DrawText(tvRenderer, "for Wii U", 1580, 75, 64, { 0, 0, 100, 200 });
            DrawText(tvRenderer, "version 6", 1610, 133, 48, { 0, 0, 100, 200 });

            if (scene == "selection_menu") {
                DrawText(tvRenderer, "Sign Up or Sign In", 600, 300, 64, textColor);
            }
            else if (scene == "sign_up") {
                DrawText(tvRenderer, "Sign Up", 850, 300, 64, textColor);
                DrawText(tvRenderer, "Enter a username and password.", 450, 400, 64, {0, 0, 0, 120});
            }
            else if (scene == "sign_in") {
                DrawText(tvRenderer, "Sign In", 850, 300, 64, textColor);
                DrawText(tvRenderer, "Enter a username and password.", 450, 400, 64, {0, 0, 0, 120});
            }
            else if (scene == "sign_up_confirm" || scene == "sign_in_confirm") {
                DrawText(tvRenderer, "Confirm", 850, 300, 64, textColor);
                DrawText(tvRenderer, ("Username: " + username).c_str(), 450, 400, 64, {0, 0, 0, 120});
                if (showpassword) DrawText(tvRenderer, ("Password: " + password).c_str(), 450, 464, 64, {0, 0, 0, 120});
                else DrawText(tvRenderer, "Password: (hidden)", 450, 464, 64, {0, 0, 0, 120});
            }
            else if (scene == "chat") {
                DrawChatBuffer(tvRenderer, 0, 40, 40, textColor);
            }
            SDL_RenderPresent(tvRenderer);
        }

        // Render DRC (GamePad) Screen
        if (drcRenderer) {
            SDL_SetRenderDrawColor(drcRenderer, themeColor.r, themeColor.g, themeColor.b, themeColor.a);
            SDL_RenderClear(drcRenderer);

            if (scene == "selection_menu") {
                DrawButtonWithText(drcRenderer, buttonTexture, button_middle_top, "Sign Up", 48);
                DrawButtonWithText(drcRenderer, buttonTexture, button_middle_bottom, "Sign In", 48);
            }
            else if (scene == "sign_up" || scene == "sign_in") {
                DrawButtonWithText(drcRenderer, buttonTexture, button_middle_top, "Username", 48);
                DrawButtonWithText(drcRenderer, buttonTexture, button_middle_bottom, "Password", 48);
                DrawButtonWithText(drcRenderer, buttonTexture, button_right_bottom, "Continue", 48);
            }
            else if (scene == "sign_up_confirm") {
                DrawButtonWithText(drcRenderer, buttonTexture, button_middle_top, "Register", 48);
                DrawButtonWithText(drcRenderer, buttonTexture, button_middle_bottom, "Show Password", 48);
            }
            else if (scene == "sign_in_confirm") {
                DrawButtonWithText(drcRenderer, buttonTexture, button_middle_top, "Log In", 48);
                DrawButtonWithText(drcRenderer, buttonTexture, button_middle_bottom, "Show Password", 48);
            }
            else if (scene == "chat") {
                DrawText(drcRenderer, "Ⓐ: Send Message", 20, 20, 48, textColor);
                DrawText(drcRenderer, "↑/↓: Scroll Chat", 20, 70, 48, textColor);
                DrawText(drcRenderer, "X: Toggle Theme", 20, 120, 48, textColor);

                DrawButtonWithText(drcRenderer, buttonTexture, button_right_bottom, "Send", 48);
            }
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
