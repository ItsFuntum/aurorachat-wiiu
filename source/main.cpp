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
#include <romfs-wiiu.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

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

void DrawText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color)
{
    // Create an SDL surface from the text
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) return;

    // Convert the surface to a texture
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) return;

    // Get texture dimensions
    int textW = 0, textH = 0;
    SDL_QueryTexture(texture, NULL, NULL, &textW, &textH);

    // Set destination rectangle and render
    SDL_Rect dstRect = { x, y, textW, textH };
    SDL_RenderCopy(renderer, texture, NULL, &dstRect);

    // Free texture
    SDL_DestroyTexture(texture);
}

// -----------------------
// Main
// -----------------------
int main(int argc, char **argv)
{
    WHBProcInit();
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    romfsInit();

    // Socket setup
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) {
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(SERVER_PORT);
        serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

        if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
            WHBLogPrintf("Failed to connect to server\n");
            WHBLogConsoleDraw();
            close(sock);
            sock = -1;
        } else {
            WHBLogPrintf("Connected to chat server\n");
            WHBLogConsoleDraw();
        }
    } else {
        WHBLogPrintf("Failed to create socket\n");
        WHBLogConsoleDraw();
    }

    // Variables
    char input[512] = "";
    int in_len = 0;
    char recvBuffer[256];

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return -1;
    }

    // Initialize TTF
    if (TTF_Init() == -1) {
        return -1;
    }
    
    SDL_Window *window = SDL_CreateWindow(NULL, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Load font
    TTF_Font *font = TTF_OpenFont("romfs:/res/FOT-RodinNTLG Pro DB.otf", 24);
    if (!font) {
        return -1;
    }

    SDL_Color white = {255, 255, 255, 255};

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

            // A = type
            if (vpad.trigger & VPAD_BUTTON_A) {
                send_chat_line(&sock, &in_len, input);
            }

            // B = backspace
            if (vpad.trigger & VPAD_BUTTON_B) {
                if (in_len > 0) input[--in_len] = '\0';
            }

            // X = shift toggle
            if (vpad.trigger & VPAD_BUTTON_X) {
                
            }

            // Y / PLUS = send
            if ((vpad.trigger & VPAD_BUTTON_PLUS)) {
                send_chat_line(&sock, &in_len, input);
            }
        }

        // Receive messages
        if (sock >= 0) {
            int len = recv(sock, recvBuffer, sizeof(recvBuffer)-1, MSG_DONTWAIT);
            if (len > 0) {
                recvBuffer[len] = '\0';
                WHBLogPrintf(recvBuffer);
            } else if (len == 0) {
                WHBLogPrintf("Connection closed by server");
                close(sock);
                sock = -1;
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black background
        SDL_RenderClear(renderer);

        SDL_Color white = {255, 255, 255, 255};
        DrawText(renderer, font, "Hello, world!", 100, 100, white);

        SDL_RenderPresent(renderer);
    }

    if (sock >= 0) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }

    romfsExit();
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    WHBProcShutdown();
    return 0;
}