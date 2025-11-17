#include <whb/proc.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <string>
#include <romfs-wiiu.h>
#include <deque>
#include <fcntl.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_syswm.h>

#define SERVER_IP "104.236.25.60"
#define SERVER_PORT 8961

std::string scene = "main";
std::string username = "";
std::string textBuffer = "";
std::string textSendType = "";

struct Theme {
    SDL_Color backgroundColor;
    SDL_Color textColor;
    std::string name;
};

std::map<int, Theme> Themes = {
    {1, {{255, 255, 255, 255}, {0, 0, 0, 255}, "Aurora White"}},
    {2, {{73, 73, 73, 255}, {0, 0, 0, 255}, "Deep Gray"}},
    {3, {{0, 26, 242, 255}, {0, 0, 0, 255}, "Homeblue Chat"}},
    {4, {{0, 0, 0, 255}, {17, 255, 0, 255}, "Hackertron Style"}},
    {5, {{23, 27, 57, 255}, {255, 255, 255, 255}, "True Dark Mode"}}
};

Theme current = Themes[1];
int currentTheme = 1;

// -----------------------
// Font system
// -----------------------
std::map<int, TTF_Font*> g_FontCache;
const char* g_FontPath = "romfs:/res/FOT-RodinNTLG Pro DB.otf";

TTF_Font* GetFont(int size)
{
    if (g_FontCache.find(size) != g_FontCache.end())
        return g_FontCache[size];

    TTF_Font* font = TTF_OpenFont(g_FontPath, size);
    if (!font)
        return nullptr;

    g_FontCache[size] = font;
    return font;
}

void FreeFonts()
{
    for (auto& f : g_FontCache)
        TTF_CloseFont(f.second);
    g_FontCache.clear();
}

void DrawText(SDL_Renderer *renderer, const char *text, int x, int y, int size, SDL_Color color)
{
    TTF_Font* font = GetFont(size);
    if (!font) return;

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) return;

    int textW = 0, textH = 0;
    SDL_QueryTexture(texture, NULL, NULL, &textW, &textH);

    SDL_Rect dstRect = { x, y, textW, textH };
    SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    SDL_DestroyTexture(texture);
}

// -----------------------
// Chat buffer
// -----------------------
std::deque<std::string> g_ChatBuffer;
int chatPosY = 0;

void AddChatLine(const std::string &msg)
{
    g_ChatBuffer.push_back(msg);
}

void DrawChatBuffer(SDL_Renderer *renderer, int startX, int startY, int lineHeight, SDL_Color color)
{
    int y = startY + chatPosY;

    for (int i = 0; i < g_ChatBuffer.size(); ++i)
    {
        DrawText(renderer, g_ChatBuffer[i].c_str(), startX, y, 24, color);
        y += lineHeight;
    }
}

// -----------------------
// Image system
// -----------------------

void DrawImage(SDL_Renderer *renderer, int x, int y, const char *file)
{
    SDL_Texture* texture = IMG_LoadTexture(renderer, file);
    if (!texture) {
        AddChatLine(std::string("Failed to load image: ") + IMG_GetError());
        return;
    }

    int w, h;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);

    SDL_Rect dstRect = { x, y, w, h };
    SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    SDL_DestroyTexture(texture);
}

// -----------------------
// Networking helpers
// -----------------------
static bool SetNonBlocking(int sock)
{
    if (sock < 0) return false;
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) flags = 0;
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK) != -1;
}

int ConnectToServer()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    SetNonBlocking(sock);

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    int res = connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (res < 0 && errno != EINPROGRESS)
    {
        AddChatLine("Immediate connect() failure.");
        close(sock);
        return -1;
    }

    // Wait up to 5 seconds for connection
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);
    struct timeval tv = {5, 0};

    res = select(sock + 1, NULL, &wfds, NULL, &tv);
    if (res <= 0)
    {
        AddChatLine("Connection timed out.");
        close(sock);
        return -1;
    }

    // Check for socket errors
    int so_error = 0;
    socklen_t len = sizeof(so_error);
    getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
    if (so_error != 0)
    {
        AddChatLine("Connection failed.");
        close(sock);
        return -1;
    }

    // Connected successfully
    SetNonBlocking(sock);
    return sock;
}

void TryReceive(int *sock)
{
    if (*sock < 0) return;

    char buf[512];
    while (true) {
        ssize_t r = recv(*sock, buf, sizeof(buf) - 1, 0);
        if (r > 0) {
            buf[r] = '\0';
            char *start = buf;
            for (char *p = buf; *p; ++p) {
                if (*p == '\n') {
                    *p = '\0';
                    AddChatLine(start);
                    start = p + 1;
                }
            }
            if (*start) AddChatLine(start);
        } else if (r == 0) {
            AddChatLine("Server disconnected.");
            close(*sock);
            *sock = -1;
            break;
        } else {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                break;
            AddChatLine("Failed to connect to aurorachat server.");
            close(*sock);
            *sock = -1;
            break;
        }
    }
}

void send_chat_line(int *sock, const char *username, const char *input)
{
    if (!input || input[0] == '\0') return;

    char sendbuf[600];
    if (username && username[0] != '\0')
        snprintf(sendbuf, sizeof(sendbuf), "<%s>: %s\n", username, input);
    else
        snprintf(sendbuf, sizeof(sendbuf), "%s\n", input);

    if (*sock >= 0) {
        ssize_t sent = send(*sock, sendbuf, strlen(sendbuf), 0);
        if (sent < 0) {
            AddChatLine("Send failed, reconnecting...");
            close(*sock);
            *sock = -1;
        }
    }

    if (*sock < 0) {
        *sock = ConnectToServer();
    }
}

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
                DrawText(tvRenderer, "v0.0.3", 1700, 120, 64, current.textColor);
                DrawText(tvRenderer, (current.name).c_str(), 820, 0, 32, current.textColor);
                DrawText(tvRenderer, "A: Change Username", 0, 20, 64, current.textColor);
                DrawText(tvRenderer, "B: Send Message", 0, 110, 64, current.textColor);
                DrawText(tvRenderer, "L: Rules", 0, 200, 64, current.textColor);
                DrawText(tvRenderer, "D-PAD: Change Theme", 0, 290, 64, current.textColor);
                DrawText(tvRenderer, ("Username: " + username).c_str(), 0, 900, 96, current.textColor);

                DrawImage(tvRenderer, 1350, 10, "romfs:/res/logo.png");
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
