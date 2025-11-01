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

#include <SDL2/SDL_ttf.h>
#include <SDL.h>
#include <src/video/wiiu/SDL_wiiuswkbd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8961

void send_chat_line(int *sock, int *in_len, char *input) {
    if (*in_len > 0) {
        if (*sock >= 0) {
            char sendbuf[600];
            if (send(*sock, sendbuf, strlen(sendbuf), 0) >= 0) {
                *in_len = 0;
                input[0] = '\0';
            }
        } else {
            WHBLogPrintf("Send failed, trying to reconnect...");

            // Close old socket
            if (*sock >= 0) close(*sock);
            *sock = -1;

            // Reconnect
            *sock = socket(AF_INET, SOCK_STREAM, 0);
            if (*sock < 0) {
                WHBLogPrintf("Failed to create socket");
                return;
            }

            struct sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(SERVER_PORT);
            serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

            if (connect(*sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == 0) {
                WHBLogPrintf("Reconnected successfully!");
            } else {
                WHBLogPrintf("Failed to reconnect, try again.");
                close(*sock);
                *sock = -1;
            }
        }
    }
}

// Store font
std::map<int, TTF_Font*> g_FontCache;
const char* g_FontPath = "romfs:/res/FOT-RodinNTLG Pro DB.otf";

TTF_Font* GetFont(int size)
{
    // If the font for this size already exists, return it
    if (g_FontCache.find(size) != g_FontCache.end())
        return g_FontCache[size];

    // Otherwise, load it
    TTF_Font* font = TTF_OpenFont(g_FontPath, size);
    if (!font) {
        return nullptr;
    }

    g_FontCache[size] = font;
    return font;
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

void FreeFonts()
{
    for (auto& f : g_FontCache)
        TTF_CloseFont(f.second);
    g_FontCache.clear();
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
    WIIU_SWKBD_Initialize();

    // Socket setup
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) {
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(SERVER_PORT);
        serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

        if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
            close(sock);
            sock = -1;
        }
    }

    // Variables
    char input[512] = "";
    int in_len = 0;
    char recvBuffer[256];
    
    SDL_Window *window = SDL_CreateWindow(NULL, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Color black = {0, 0, 0, 255};

    std::string scene = "main";

    std::string username = "";

    // Main loop
    while (WHBProcIsRunning()) {
        VPADStatus vpad;
        VPADReadError error;
        VPADRead(VPAD_CHAN_0, &vpad, 1, &error);

        if (error == VPAD_READ_SUCCESS) {
            // D-pad navigation
            if (vpad.trigger & VPAD_BUTTON_UP) {

            }
            if (vpad.trigger & VPAD_BUTTON_DOWN) {
                
            }
            if (vpad.trigger & VPAD_BUTTON_LEFT) {
                
            }
            if (vpad.trigger & VPAD_BUTTON_RIGHT) {
                
            }

            // A = Change Username
            if (vpad.trigger & VPAD_BUTTON_A) {
                
            }

            // B = Send Message
            if (vpad.trigger & VPAD_BUTTON_B) {
                WIIU_SWKBD_ShowScreenKeyboard(nullptr, window);
            }

            // L and X = Rules Scene Toggle
            if ((vpad.trigger & VPAD_BUTTON_L) && scene == "main") {
                scene = "rules";
            } else if ((vpad.trigger & VPAD_BUTTON_X) && scene == "rules") {
                scene = "main";
            }
        }

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
        }
        else if (scene == "rules") {
            DrawText(renderer, "(Press X to Go Back)", 0, 20, 64, black);
        }

        SDL_RenderPresent(renderer);
    }

    if (sock >= 0) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }

    WIIU_SWKBD_Finalize();
    romfsExit();
    FreeFonts();
    TTF_Quit();
    SDL_Quit();
    WHBProcShutdown();
    return 0;
}