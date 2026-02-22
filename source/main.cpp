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
#include "theme.h"

// Used in multiple files, so declared here
// -------------------------
std::string clientVersion = "5.3";

std::string username = "";
std::string password = "";

int fontSize = 48;
int maxWidth = 1920 - 40;

int sock = ConnectToTCPServer();

bool connectionLost = false;

SDL_Window *tvWindow = NULL;
SDL_Window *drcWindow = NULL;
SDL_Renderer *tvRenderer = NULL;
SDL_Renderer *drcRenderer = NULL;

// TV colors
SDL_Color tvBackgroundColor;
SDL_Color tvTextColor;

// DRC colors
SDL_Color drcBackgroundColor;
SDL_Color drcTextColor;

// Logo
SDL_Color logoColor;
// -------------------------

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

    std::string serverResponse = "";
    std::string failedReason = "";

    // Keyboard Text Input Buffer
    std::string textBuffer = "";

    bool showpassword = false;

    connect_to_api();

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

    // Set vsync hint before creating windows
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    // TV Window (primary display)
    tvWindow = SDL_CreateWindow("TV", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        1280, 720,  // Use 720p resolution
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_WIIU_TV_ONLY);
    if (tvWindow) {
        tvRenderer = SDL_CreateRenderer(tvWindow, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    }

    // GamePad Window
    drcWindow = SDL_CreateWindow("DRC",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        854, 480,  // Native GamePad resolution
        SDL_WINDOW_WIIU_GAMEPAD_ONLY | SDL_WINDOW_WIIU_PREVENT_SWAP);
    if (drcWindow) {
        drcRenderer = SDL_CreateRenderer(drcWindow, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    }

    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

    ApplyTheme(0);

    AddChatLine(tvRenderer, "-chat-", fontSize, tvTextColor, maxWidth);

    // Discord QR Code Texture
    SDL_Texture* discordTexture = LoadImage(drcRenderer, "romfs:/res/QRCode_Discord.png");

    int originalW, originalH;
    SDL_QueryTexture(discordTexture, NULL, NULL, &originalW, &originalH);

    float discordScale = 0.75f;

    SDL_Rect discordRect;
    discordRect.x = 20;
    discordRect.y = 305;
    discordRect.w = originalW * discordScale;
    discordRect.h = originalH * discordScale;

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

    SDL_Rect button_left_bottom = { 854, 480, 0, 0 };
    button_left_bottom.w = bw;
    button_left_bottom.h = bh;
    button_left_bottom.x = 0;
    button_left_bottom.y = 480 - button_left_bottom.h;

    Uint32 lastTicks = 0;
    const int AXIS_DEADZONE = 8000;  // deadzone for joystick
    const float MAX_SPEED = 1000.0f;  // pixels per second when stick is fully pushed

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
        if (scene == "chat") {
            Uint32 now = SDL_GetTicks();
            float deltaSec = (now - lastTicks) / 1000.0f;
            lastTicks = now;

            if (gController) {
                // ANALOG STICK
                Sint16 axisY = SDL_GameControllerGetAxis(gController, SDL_CONTROLLER_AXIS_LEFTY);

                if (axisY > AXIS_DEADZONE || axisY < -AXIS_DEADZONE) {
                    float norm = axisY / 32767.0f;
                    float move = norm * MAX_SPEED * deltaSec;
                    chatPosY -= (int)move;
                }

                // D-PAD
                if (SDL_GameControllerGetButton(gController, SDL_CONTROLLER_BUTTON_DPAD_UP)) {
                    chatPosY += (int)(MAX_SPEED * deltaSec);
                }

                if (SDL_GameControllerGetButton(gController, SDL_CONTROLLER_BUTTON_DPAD_DOWN)) {
                    chatPosY -= (int)(MAX_SPEED * deltaSec);
                }
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
                        std::string reply = make_account(username.c_str(), password.c_str());

                        if (reply.find("\"data\":\"USR_CREATED\"") != std::string::npos) {
                            serverResponse.clear();
                            scene = "register_success";
                        }
                        else if (reply.find("\"data\":\"USR_IN_USE\"") != std::string::npos) {
                            serverResponse.clear();
                            failedReason = "Username already in use.";
                            scene = "failed";
                        }
                        else {
                            // Extract the "data" value
                            size_t start = reply.find("\"data\":\"");
                            size_t end = reply.find("\"", start + 8);
                            if (start != std::string::npos && end != std::string::npos) {
                                serverResponse = reply.substr(start + 8, end - (start + 8));
                            }
                            else {
                                serverResponse = "Unable to parse server response.";
                            }

                            failedReason = "An Unknown Error Occurred.";
                            scene = "failed";
                        }
                    }
                    else if (scene == "sign_in_confirm") {
                        std::string reply = login_account(username.c_str(), password.c_str());

                        if (reply.find("\"data\":\"LOGIN_OK\"") != std::string::npos) {
                            serverResponse.clear();

                            std::string welcome = "Welcome to aurorachat, " + username + "!";
                            AddChatLine(tvRenderer, welcome.c_str(), fontSize, tvTextColor, maxWidth);

                            scene = "chat";
                        }
                        else if (reply.find("\"data\":\"LOGIN_FAKE_ACC\"") != std::string::npos) {
                            serverResponse.clear();
                            failedReason = "User not found.";
                            scene = "failed";
                        }
                        else if (reply.find("\"data\":\"LOGIN_WRONG_PASS\"") != std::string::npos) {
                            serverResponse.clear();
                            failedReason = "Incorrect password.";
                            scene = "failed";
                        }
                        else if (reply.find("\"data\":\"frick you you're BANNED\"") != std::string::npos) {
                            serverResponse.clear();
                            failedReason = "Banned User.";
                            scene = "failed";
                        }
                        else {
                            // Extract the "data" value
                            size_t start = reply.find("\"data\":\"");
                            size_t end = reply.find("\"", start + 8);
                            if (start != std::string::npos && end != std::string::npos) {
                                serverResponse = reply.substr(start + 8, end - (start + 8));
                            }
                            else {
                                serverResponse = "Unable to parse server response.";
                            }

                            failedReason = "An Unknown Error Occurred.";
                            scene = "failed";
                        }
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
                    if (scene == "sign_up") {
                        if (username.empty() || password.empty()) {
                            failedReason = "Username and password cannot be empty.";
                            scene = "failed";
                        }
                        else {
                            scene = "sign_up_confirm";
                        }
                    }
                    else if (scene == "sign_in") {
                        if (username.empty() || password.empty()) {
                            failedReason = "Username and password cannot be empty.";
                            scene = "failed";
                        }
                        else {
                            scene = "sign_in_confirm";
                        }
                    }
                    else if (scene == "chat" && rulesPage == 0) {
                        if (connectionLost) {
                            ReconnectToTCPServer();
                        } else {
                            textSendType = "message";
                            SDL_WiiUSetSWKBDHintText("Say something...");
                            SDL_StartTextInput();
                        }
                    }
                }
                else if (PointInRect(mx, my, button_left_bottom)) {
                    if (scene == "sign_up" || scene == "sign_in" || scene == "failed" || scene == "register_success" || scene == "chat" && rulesPage == 0) {
                        if (scene == "chat") {
                            username = "";
                            password = "";
                        }

                        scene = "selection_menu";
                    }
                    else if (scene == "sign_up_confirm") scene = "sign_up";
                    else if (scene == "sign_in_confirm") scene = "sign_in";
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
        TryReceive(&sock, tvRenderer, fontSize, tvTextColor, maxWidth);

        // Render TV Screen
        if (tvRenderer) {
            SDL_SetRenderDrawColor(tvRenderer, tvBackgroundColor.r, tvBackgroundColor.g, tvBackgroundColor.b, tvBackgroundColor.a);
            SDL_RenderClear(tvRenderer);

            DrawText(tvRenderer, "Aurorachat", 1500, 20, 64, logoColor);
            DrawText(tvRenderer, "for Wii U", 1580, 75, 64, logoColor);
            DrawText(tvRenderer, ("version " + clientVersion).c_str(), 1610, 133, 48, logoColor);

            if (scene == "selection_menu") {
                DrawText(tvRenderer, "Sign Up or Sign In", 600, 300, 64, tvTextColor);
            }
            else if (scene == "sign_up") {
                DrawText(tvRenderer, "Sign Up", 850, 300, 64, tvTextColor);
                DrawText(tvRenderer, "Enter a username and password.", 450, 400, 64, tvTextColor);
            }
            else if (scene == "sign_in") {
                DrawText(tvRenderer, "Sign In", 850, 300, 64, tvTextColor);
                DrawText(tvRenderer, "Enter a username and password.", 450, 400, 64, tvTextColor);
            }
            else if (scene == "sign_up_confirm" || scene == "sign_in_confirm") {
                DrawText(tvRenderer, "Confirm", 850, 300, 64, tvTextColor);
                DrawText(tvRenderer, ("Username: " + username).c_str(), 450, 400, 64, tvTextColor);
                if (showpassword) DrawText(tvRenderer, ("Password: " + password).c_str(), 450, 464, 64, tvTextColor);
                else DrawText(tvRenderer, "Password: (hidden)", 450, 464, 64, tvTextColor);
            }
            else if (scene == "chat") {
                DrawChatBuffer(tvRenderer, 40, 40);
            }
            else if (scene == "register_success") {
                DrawText(tvRenderer, "Account created successfully!", 500, 300, 64, tvTextColor);
                DrawText(tvRenderer, "Press A to go to the chat screen.", 250, 400, 64, tvTextColor);
                DrawText(tvRenderer, "Press B to go back to the account screen.", 250, 500, 64, tvTextColor);

                if (!serverResponse.empty())
                    DrawText(tvRenderer, ("Debug: " + serverResponse).c_str(), 0, 1040, 32, tvTextColor);
            }
            else if (scene == "failed") {
                DrawText(tvRenderer, failedReason.c_str(), 250, 300, 64, tvTextColor);
                DrawText(tvRenderer, "Press B to go back to the account screen.", 250, 400, 64, tvTextColor);

                if (!serverResponse.empty())
                    DrawText(tvRenderer, ("Debug: " + serverResponse).c_str(), 0, 1040, 32, tvTextColor);
            }
            SDL_RenderPresent(tvRenderer);
        }

        // Render DRC (GamePad) Screen
        if (drcRenderer) {
            SDL_SetRenderDrawColor(drcRenderer, drcBackgroundColor.r, drcBackgroundColor.g, drcBackgroundColor.b, drcBackgroundColor.a);
            SDL_RenderClear(drcRenderer);

            if (scene != "selection_menu" && rulesPage == 0) {
                if (scene == "chat") DrawButtonWithText(drcRenderer, buttonTexture, button_left_bottom, "Log Out", 48);
                else DrawButtonWithText(drcRenderer, buttonTexture, button_left_bottom, "Back", 48);
            }

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
                if (rulesPage == 1) {
                    DrawText(drcRenderer, "Ⓨ: Next Page", 20, 20, 20, drcTextColor);

                    DrawText(drcRenderer, "1. No racist, sexist, homophobic, or other", 20, 60, 20, drcTextColor);
                    DrawText(drcRenderer, "prejudiced language or behavior, whether it's", 20, 80, 20, drcTextColor);
                    DrawText(drcRenderer, "aimed at another user or not.", 20, 100, 20, drcTextColor);

                    DrawText(drcRenderer, "2. No asking for or sharing personal info of", 20, 150, 20, drcTextColor);
                    DrawText(drcRenderer, "yourself or anyone else. This includes name,", 20, 170, 20, drcTextColor);
                    DrawText(drcRenderer, "age, gender, location, phone number, email", 20, 190, 20, drcTextColor);
                    DrawText(drcRenderer, "address, or any other personally identifiable", 20, 210, 20, drcTextColor);
                    DrawText(drcRenderer, "information. Doxxing (or the threat of doing so)", 20, 230, 20, drcTextColor);
                    DrawText(drcRenderer, "is grounds for a ban.", 20, 250, 20, drcTextColor);
                }
                else if (rulesPage == 2) {
                    DrawText(drcRenderer, "Ⓨ: Next Page", 20, 20, 20, drcTextColor);

                    DrawText(drcRenderer, "3. No overly violent or sexual behavior or", 20, 60, 20, drcTextColor);
                    DrawText(drcRenderer, "language, including threats of violence or harm", 20, 80, 20, drcTextColor);
                    DrawText(drcRenderer, "of ANY kind. This includes threats or", 20, 100, 20, drcTextColor);
                    DrawText(drcRenderer, "discussion of harming yourself.", 20, 120, 20, drcTextColor);

                    DrawText(drcRenderer, "4. No political discussion. Usernames of", 20, 170, 20, drcTextColor);
                    DrawText(drcRenderer, "political figures are allowed (with some", 20, 190, 20, drcTextColor);
                    DrawText(drcRenderer, "exceptions), but any language that could incite", 20, 210, 20, drcTextColor);
                    DrawText(drcRenderer, "arguments may get you banned.", 20, 230, 20, drcTextColor);

                    DrawText(drcRenderer, "5. No impersonation of developers,", 20, 280, 20, drcTextColor);
                    DrawText(drcRenderer, "moderators, admin, or any other Aurorachat", 20, 300, 20, drcTextColor);
                    DrawText(drcRenderer, "staff. Additionally, ANY impersonation for the", 20, 320, 20, drcTextColor);
                    DrawText(drcRenderer, "sake of harrassing another user is not allowed.", 20, 340, 20, drcTextColor);
                }
                else if (rulesPage == 3) {
                    DrawText(drcRenderer, "Ⓨ: Close Rules", 20, 20, 20, drcTextColor);

                    DrawText(drcRenderer, "6. No spamming.", 20, 60, 20, drcTextColor);
                    DrawText(drcRenderer, "7. No hunting.", 20, 80, 20, drcTextColor);
                    DrawText(drcRenderer, "8. No hackertron", 20, 100, 20, drcTextColor);

                    DrawText(drcRenderer, "Friend code sharing is allowed, but please do", 20, 150, 20, drcTextColor);
                    DrawText(drcRenderer, "not harrass or pressure other users for their", 20, 170, 20, drcTextColor);
                    DrawText(drcRenderer, "friend codes.", 20, 190, 20, drcTextColor);

                    DrawText(drcRenderer, "Want access to the rest of Unitendo? Join our", 20, 240, 20, drcTextColor);
                    DrawText(drcRenderer, "official Discord server here:", 20, 260, 20, drcTextColor);
                    DrawText(drcRenderer, "discord.gg/dCSgz7KERv", 20, 280, 20, drcTextColor);

                    SDL_RenderCopy(drcRenderer, discordTexture, NULL, &discordRect);

                    DrawText(drcRenderer, "We are not accepting ban appeals at this time.", 20, 455, 20, drcTextColor);
                }
                else {
                    if (connectionLost) {
                        DrawText(drcRenderer, "Ⓐ: Reconnect to server", 20, 0, 48, drcTextColor);
                    }
                    else {
                        DrawText(drcRenderer, "Ⓐ: Send Message", 20, 0, 48, drcTextColor);
                    }
                    DrawText(drcRenderer, "↑/↓: Scroll Chat", 20, 50, 48, drcTextColor);
                    DrawText(drcRenderer, "Ⓨ: View Rules", 20, 100, 48, drcTextColor);
                    DrawText(drcRenderer, "L/R: Toggle Theme", 20, 150, 48, drcTextColor);
                    DrawText(drcRenderer, "X: Reverse Theme", 20, 200, 48, drcTextColor);
                    DrawText(drcRenderer, "Current Theme:", 20, 260, 48, drcTextColor);
                    if (isThemeReversed)
                        DrawText(drcRenderer, (std::string(themes[currentTheme].name) + " (reversed)").c_str(), 20, 310, 48, drcTextColor);
                    else {
                        DrawText(drcRenderer, themes[currentTheme].name, 20, 310, 48, drcTextColor);
                    }
                    if (connectionLost) {
                        DrawButtonWithText(drcRenderer, buttonTexture, button_right_bottom, "Reconnect", 48);
                    } else {
                        DrawButtonWithText(drcRenderer, buttonTexture, button_right_bottom, "Send", 48);
                    }
                }
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
    SDL_CloseAudio();
    SDL_Quit();
    WHBProcShutdown();
    return 0;
}
