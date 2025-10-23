<<<<<<< HEAD
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
#include <stdio.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8961

// -----------------------
// Keyboard layout (5 rows)
// -----------------------
static const char *KB_ROWS_LOWER[] = {
    "0123456789",
    "qwertyuiop",
    "asdfghjkl;",
    "zxcvbnm,./",
    " |" // | = Send button
};

static const char *KB_ROWS_UPPER[] = {
    "=!\"#$%&/()",
    "QWERTYUIOP",
    "ASDFGHJKL:",
    "ZXCVBNM<>?",
    " |" // | = Send button
};

static int kb_rows = 5;
static int kb_cols[5];

// -----------------------
// Chat history
// -----------------------
#define CHAT_HISTORY_LINES 8
static char chat_history[CHAT_HISTORY_LINES][128];
static int chat_count = 0;

void add_chat_line(const char *msg) {
    if (chat_count < CHAT_HISTORY_LINES) {
        strncpy(chat_history[chat_count++], msg, sizeof(chat_history[0])-1);
    } else {
        for (int i = 1; i < CHAT_HISTORY_LINES; ++i)
            strcpy(chat_history[i-1], chat_history[i]);
        strncpy(chat_history[CHAT_HISTORY_LINES-1], msg, sizeof(chat_history[0])-1);
    }
}

// -----------------------
// Render keyboard below chat
// -----------------------
void render_keyboard(const char *input, int in_len, int shift, int sel_row, int sel_col)
{
    // "Clear" console by printing blank lines
    for (int i = 0; i < 6; i++)
        WHBLogPrintf("\n");

    // Print chat history
    WHBLogPrintf("=== AuroraChat ===\n");
    for (int i = 0; i < chat_count; ++i)
        WHBLogPrintf("%s\n", chat_history[i]);

    WHBLogPrintf("------------------\n");

    // Input line
    WHBLogPrintf("INPUT: %s\n", input);
    WHBLogPrintf("------------------\n");

    // Keyboard rows
    for (int r = 0; r < kb_rows; ++r) {
        const char *rowStr = shift ? KB_ROWS_UPPER[r] : KB_ROWS_LOWER[r];
        int cols = kb_cols[r]; if (cols < 1) cols = 1;

        char line[128]; int pos = 0;
        for (int c = 0; c < cols; ++c) {
            char ch = rowStr[c];
            if (r == sel_row && c == sel_col) {
                if (ch == ' ')
                    pos += snprintf(line + pos, sizeof(line)-pos, "[SPACE]");
                else if (ch == '|')
                    pos += snprintf(line + pos, sizeof(line)-pos, "[SEND]");
                else
                    pos += snprintf(line + pos, sizeof(line)-pos, "[%c]", ch);
            } else {
                if (ch == ' ')
                    pos += snprintf(line + pos, sizeof(line)-pos, " SPACE ");
                else if (ch == '|')
                    pos += snprintf(line + pos, sizeof(line)-pos, " SEND ");
                else
                    pos += snprintf(line + pos, sizeof(line)-pos, " %c ", ch);
            }
        }
        WHBLogPrintf("%s\n", line);
    }

    WHBLogPrintf("\n");
    WHBLogPrintf("\nD-PAD=move | A=type | B=backspace | X=shift");
    WHBLogPrintf("\nPLUS=quick send | HOME=exit");

    WHBLogConsoleDraw();
}

void send_chat_line(int *sock, int *in_len, char *input) {
    if (*in_len > 0) {
        if (*sock >= 0) {
            char sendbuf[600];
            snprintf(sendbuf, sizeof(sendbuf), "%s\n", input);
            if (send(*sock, sendbuf, strlen(sendbuf), 0) >= 0) {
                *in_len = 0;
                input[0] = '\0';
            }
        } else {
            add_chat_line("Send failed, trying to reconnect...");

            // Close old socket
            if (*sock >= 0) close(*sock);
            *sock = -1;

            // Reconnect
            *sock = socket(AF_INET, SOCK_STREAM, 0);
            if (*sock < 0) {
                add_chat_line("Failed to create socket");
                return;
            }

            struct sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(SERVER_PORT);
            serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

            if (connect(*sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == 0) {
                add_chat_line("Reconnected successfully!");
            } else {
                add_chat_line("Failed to reconnect, try again.");
                close(*sock);
                *sock = -1;
            }
        }
    }
}

// -----------------------
// Main
// -----------------------
int main(int argc, char **argv)
{
    WHBProcInit();
    WHBLogConsoleInit();
    WHBLogConsoleSetColor(0x000000FF); // Black background
    WHBLogConsoleDraw();

    // Initialize kb_cols safely
    for (int r = 0; r < kb_rows; ++r) {
        kb_cols[r] = (int)strlen(KB_ROWS_LOWER[r]);
        if (kb_cols[r] < 1) kb_cols[r] = 1;
    }

    // Socket setup
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) {
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(SERVER_PORT);
        serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

        if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
            add_chat_line("Failed to connect to server\n");
            WHBLogConsoleDraw();
            close(sock);
            sock = -1;
        } else {
            add_chat_line("Connected to chat server\n");
            WHBLogConsoleDraw();
        }
    } else {
        add_chat_line("Failed to create socket\n");
        WHBLogConsoleDraw();
    }

    // Variables
    char input[512] = "";
    int in_len = 0;
    int sel_row = 0, sel_col = 0, shift = 0;
    char recvBuffer[256];

    // Main loop
    while (WHBProcIsRunning()) {
        VPADStatus vpad;
        int error;
        VPADRead(0, &vpad, 1, &error);

        if (error == VPAD_READ_SUCCESS) {
            if (vpad.hold & VPAD_BUTTON_HOME) break;

            // D-pad navigation
            if (vpad.trigger & VPAD_BUTTON_UP) {
                sel_row = (sel_row - 1 + kb_rows) % kb_rows; if (sel_col >= kb_cols[sel_row]) sel_col = kb_cols[sel_row]-1;
            }
            if (vpad.trigger & VPAD_BUTTON_DOWN) {
                sel_row = (sel_row + 1) % kb_rows; if (sel_col >= kb_cols[sel_row]) sel_col = kb_cols[sel_row]-1;
            }
            if (vpad.trigger & VPAD_BUTTON_LEFT) {
                sel_col = (sel_col - 1 + kb_cols[sel_row]) % kb_cols[sel_row];
            }
            if (vpad.trigger & VPAD_BUTTON_RIGHT) {
                sel_col = (sel_col + 1) % kb_cols[sel_row];
            }

            // A = type
            if (vpad.trigger & VPAD_BUTTON_A) {
                const char *rowStr = shift ? KB_ROWS_UPPER[sel_row] : KB_ROWS_LOWER[sel_row];
                char ch = rowStr[sel_col];
                if (ch == '|') {
                    send_chat_line(&sock, &in_len, input);
                } else if (in_len < (int)sizeof(input)-2) {
                    input[in_len++] = ch;
                    input[in_len] = '\0';
                }
            }

            // B = backspace
            if (vpad.trigger & VPAD_BUTTON_B) {
                if (in_len > 0) input[--in_len] = '\0';
            }

            // X = shift toggle
            if (vpad.trigger & VPAD_BUTTON_X) {
                shift = !shift; if (sel_col >= kb_cols[sel_row]) sel_col = kb_cols[sel_row]-1;
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
                add_chat_line(recvBuffer);
            } else if (len == 0) {
                add_chat_line("Connection closed by server");
                close(sock);
                sock = -1;
            }
        }
    
        // Draw chat + keyboard
        render_keyboard(input, in_len, shift, sel_row, sel_col);
    
        OSSleepTicks(OSMillisecondsToTicks(150));
    }

    if (sock >= 0) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }

    WHBLogConsoleFree();
    WHBProcShutdown();
    return 0;
}
=======
// aurorachat
// Author: mii-man
// Description: A chatting application for 3DS

// yes I did copy basically the entire hbchat codebase here and call it a day because ain't no way your original base was ever gonna work :sob:

#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <malloc.h>

#include <3ds/applets/swkbd.h>

#include <citro2d.h>

C2D_TextBuf sbuffer;
C2D_Text stext;

C2D_TextBuf chatbuffer;
C2D_Text chat;

char chatstring[6000] = "-chat-";
char usernameholder[64];

float chatscroll = 20;

int scene = 1;

bool inacc = false;



int main(int argc, char **argv) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    sbuffer = C2D_TextBufNew(4096);
    chatbuffer = C2D_TextBufNew(4096);


    C2D_TextParse(&chat, chatbuffer, chatstring);
    C2D_TextOptimize(&chat);


    u32 *soc_buffer = memalign(0x1000, 0x100000);
    if (!soc_buffer) {
        // placeholder
    }
    if (socInit(soc_buffer, 0x100000) != 0) {
        // placeholder
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        // placeholder
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(3071); // mmm the entire hbchat codebase and its remains
    server.sin_addr.s_addr = inet_addr(""); // put a server here

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != 0) {
        // placeholder
    }

    char username[11];

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000; // 10ms

    char buffer[512];
    while (aptMainLoop()) {
        gspWaitForVBlank();
        hidScanInput();

        if (hidKeysDown() & KEY_A) {
            char message[64];
            char input[64];
            SwkbdState swkbd;
            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, 11);
            swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT);
            swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);

            SwkbdButton button = swkbdInputText(&swkbd, username, sizeof(username)); 
        }

        if (hidKeysDown() & KEY_B) {
            char message[64];
            char msg[128];

            char input[80];
            SwkbdState swkbd;
            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, 80);
            swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT);
            swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY, 0, 0);

            SwkbdButton button = swkbdInputText(&swkbd, message, sizeof(message));
            if (button == SWKBD_BUTTON_CONFIRM) {
                if (inacc) {
                    sprintf(msg, "<%s>: %s", username, message);
                }
                if (!inacc) {
                    sprintf(msg, "<%s>: %s", username, message);
                }
                send(sock, msg, strlen(msg), 0);
                printf("Message sent!\n");
            }
        }

        fd_set readfds;
        struct timeval timeout;

        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; // 10ms

        if (select(sock + 1, &readfds, NULL, NULL, &timeout) > 0) {
            ssize_t len = recv(sock, buffer, sizeof(buffer)-1, 0);
            if (len > 0) {
                buffer[len] = '\0';

                sprintf(chatstring, "%s\n%s", chatstring, buffer);

                C2D_TextParse(&chat, chatbuffer, chatstring);
                C2D_TextOptimize(&chat);
                chatscroll = chatscroll - 25;

                const char* parseResult = C2D_TextParse(&chat, chatbuffer, chatstring);
                if (parseResult != NULL && *parseResult != '\0') {
                    chatbuffer = C2D_TextBufResize(chatbuffer, 8192);
                    if (chatbuffer) {
                        C2D_TextBufClear(chatbuffer);
                        C2D_TextParse(&chat, chatbuffer, chatstring);
                    }
                }
                C2D_TextOptimize(&chat);
            }
        }


        if (strlen(chatstring) > 3500) {
            strcpy(chatstring, "-chat-\n");
            C2D_TextBufClear(chatbuffer);
            C2D_TextParse(&chat, chatbuffer, chatstring);
            C2D_TextOptimize(&chat);
            chatscroll = 20;
        }


        if (hidKeysDown() & KEY_START) break;

        if (hidKeysHeld() & KEY_CPAD_DOWN) {
            chatscroll = chatscroll - 5;
        }

        if (hidKeysHeld() & KEY_CPAD_UP) {
            chatscroll = chatscroll + 5;
        }

        if ((hidKeysDown() & KEY_L) && scene == 1) {
            scene = 2;
        }

        if ((hidKeysDown() & KEY_X) && scene == 2) {
            scene = 1;
        }


        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, C2D_Color32(255, 255, 255, 255));
        C2D_SceneBegin(top);


        if (scene == 1) {
            C2D_TextBufClear(sbuffer);
            C2D_TextParse(&stext, sbuffer, "aurorachat");
            C2D_TextOptimize(&stext);

            C2D_DrawText(&stext, 0, 250.0f, 0.0f, 0.5f, 1.0f, 1.0f);

            sprintf(usernameholder, "%s %s", "Username:", username);

            C2D_TextBufClear(sbuffer);
            C2D_TextParse(&stext, sbuffer, usernameholder);
            C2D_TextOptimize(&stext);

            C2D_DrawText(&stext, 0, 0.0f, 200.0f, 0.5f, 1.0f, 1.0f);


            C2D_TextBufClear(sbuffer);
            C2D_TextParse(&stext, sbuffer, "v0.0.1");
            C2D_TextOptimize(&stext);

            C2D_DrawText(&stext, 0, 352.0f, 25.0f, 0.5f, 0.6f, 0.6f);



            C2D_TextBufClear(sbuffer);
            C2D_TextParse(&stext, sbuffer, "A: Change Username\nB: Send Message\nL: Rules)");
            C2D_TextOptimize(&stext);

            C2D_DrawText(&stext, 0, 0.0f, 0.0f, 0.5f, 0.6f, 0.6f);

        }


        if (scene == 2) {
            C2D_TextBufClear(sbuffer);
            C2D_TextParse(&stext, sbuffer, "(Press X to Go Back)\n\nRule 1: No Spamming\n\nRule 2: No Swearing\n\nRule 3: No Impersonating\n\nRule 4: No Politics\n\nAll of these could result in a ban.");
            C2D_TextOptimize(&stext);

            C2D_DrawText(&stext, 0, 0.0f, 0.0f, 0.5f, 0.6f, 0.6f);
        }


        C2D_TargetClear(bottom, C2D_Color32(255, 255, 255, 255));
        C2D_SceneBegin(bottom);

        C2D_DrawText(&chat, C2D_WordWrap, 0.0f, chatscroll, 0.5f, 0.5f, 0.5f, 290.0f);



        C3D_FrameEnd(0);

    }
    closesocket(sock);
    socExit();
    gfxExit();
    return 0;
}
>>>>>>> upstream/main
