#include <coreinit/thread.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <whb/proc.h>
#include <vpad/input.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <map>
#include <string>
#include <romfs-wiiu.h>
#include <deque>
#include <fcntl.h>
#include <errno.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_syswm.h>

#define SERVER_IP "104.236.25.60"
#define SERVER_PORT 8961

// -----------------------
// Chat buffer
// -----------------------
std::deque<std::string> g_ChatBuffer;
const size_t MAX_CHAT_LINES = 10;

void AddChatLine(const std::string &msg)
{
    if (g_ChatBuffer.size() >= MAX_CHAT_LINES)
        g_ChatBuffer.pop_front();
    g_ChatBuffer.push_back(msg);
}

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

void DrawChatBuffer(SDL_Renderer *renderer, int startX, int startY, int lineHeight, SDL_Color color)
{
    int y = startY;
    for (const auto &line : g_ChatBuffer)
    {
        DrawText(renderer, line.c_str(), startX, y, 48, color);
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
    if (res != 0)
    {
        if (errno != EINPROGRESS)
        {
            AddChatLine("Failed to connect to server.");
            close(sock);
            sock = -1;
        }
    }
    return sock;
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

// -----------------------
// Main
// -----------------------
int main(int argc, char **argv)
{
    WHBProcInit();
    SDL_Init(SDL_INIT_VIDEO);
    romfsInit();
    TTF_Init();
    IMG_Init(IMG_INIT_PNG);

    int sock = ConnectToServer();

    char input[512] = "";
    SDL_Window *window = SDL_CreateWindow(NULL, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_Color black = {0, 0, 0, 255};
    std::string scene = "main";
    std::string username = "";
    std::string textBuffer = "";
    std::string textSendType = "";

    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

    AddChatLine("-chat-");

    VPADStatus vpad;
    VPADReadError error;

    while (WHBProcIsRunning()) {
        VPADRead(VPAD_CHAN_0, &vpad, 1, &error);
        SDL_WiiUSetSWKBDVPAD(&vpad);

        if (error == VPAD_READ_SUCCESS) {
            if ((vpad.trigger & VPAD_BUTTON_A) && scene == "main") {
                textSendType = "username";
                SDL_WiiUSetSWKBDInitialText(username.c_str());
                SDL_StartTextInput();
            }

            if ((vpad.trigger & VPAD_BUTTON_B) && scene == "main") {
                textSendType = "message";
                SDL_StartTextInput();
            }

            if ((vpad.trigger & VPAD_BUTTON_L) && scene == "main")
                scene = "rules";
            else if ((vpad.trigger & VPAD_BUTTON_X) && scene == "rules")
                scene = "main";
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_TEXTINPUT)
                textBuffer += event.text.text;
            else if (event.type == SDL_SYSWMEVENT) {
                if (event.syswm.msg->msg.wiiu.event == SDL_WIIU_SYSWM_SWKBD_OK_FINISH_EVENT) {
                    if (!textBuffer.empty()) {
                        if (textSendType == "message") {
                            strncpy(input, textBuffer.c_str(), sizeof(input) - 1);
                            input[sizeof(input) - 1] = '\0';
                            send_chat_line(&sock, username.c_str(), input);
                        } else if (textSendType == "username") {
                            username = textBuffer;
                        }
                        textBuffer.clear();
                        textSendType.clear();
                    }
                    SDL_StopTextInput();
                }
            }
        }

        // Handle incoming messages
        TryReceive(&sock);

        // Render
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        if (scene == "main") {
            DrawText(renderer, "aurorachat", 1300, 10, 96, black);
            DrawText(renderer, "v0.0.2", 1700, 120, 64, black);
            DrawText(renderer, "A: Change Username", 0, 20, 64, black);
            DrawText(renderer, "B: Send Message", 0, 110, 64, black);
            DrawText(renderer, "L: Rules", 0, 200, 64, black);
            DrawText(renderer, ("Username: " + username).c_str(), 0, 900, 96, black);

            DrawImage(renderer, 1350, 10, "romfs:/res/logo.png");
            DrawChatBuffer(renderer, 0, 300, 60, black);
        }
        else if (scene == "rules") {
            DrawText(renderer, "(Press X to Go Back)", 0, 20, 64, black);
            DrawText(renderer, "Rule 1: No Spamming", 0, 200, 64, black);
            DrawText(renderer, "Rule 2: No Swearing", 0, 380, 64, black);
            DrawText(renderer, "Rule 3: No Impersonating", 0, 560, 64, black);
            DrawText(renderer, "Rule 4: No Politics", 0, 740, 64, black);
            DrawText(renderer, "Breaking rules may result in a ban", 0, 920, 64, black);
        }

        SDL_RenderPresent(renderer);
    }

    if (sock >= 0) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }

    IMG_Quit();
    romfsExit();
    FreeFonts();
    TTF_Quit();
    SDL_Quit();
    WHBProcShutdown();
    return 0;
}