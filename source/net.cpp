#include "net.h"

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