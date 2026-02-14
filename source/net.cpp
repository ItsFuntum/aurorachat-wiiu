#include "net.h"

static bool SetNonBlocking(int sock)
{
    if (sock < 0) return false;
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) flags = 0;
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK) != -1;
}

int ConnectToTCPServer()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT_TCP);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        close(sock);
        return -1;
    }

    SetNonBlocking(sock);
    return sock;
}

int ConnectToHTTPServer()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT_HTTP);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        close(sock);
        return -1;
    }

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

bool send_api_request(const std::string& jsonBody)
{
    int sock = ConnectToHTTPServer();
    if (sock < 0) return false;

    int bodyLen = jsonBody.length();

    char request[2048];
    int reqLen = snprintf(request, sizeof(request),
        "POST /api HTTP/1.1\r\n"
        "Host: 104.236.25.60:3072\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        bodyLen,
        jsonBody.c_str()
    );

    // ---- Send (handle partial send properly) ----
    int totalSent = 0;
    while (totalSent < reqLen) {
        int sent = send(sock, request + totalSent, reqLen - totalSent, 0);
        if (sent <= 0) {
            close(sock);
            return false;
        }
        totalSent += sent;
    }

    // ---- Read full response ----
    char buffer[1024];
    while (recv(sock, buffer, sizeof(buffer), 0) > 0) {
        // Optional: store response if needed
    }

    close(sock);
    return true;
}

std::string json_escape(const char* input)
{
    std::string out;
    for (const char* p = input; *p; ++p) {
        switch (*p) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += *p; break;
        }
    }
    return out;
}

bool make_account(const char* username, const char* password)
{
    std::string body =
        "{\"cmd\":\"MAKEACC\",\"username\":\"" +
        json_escape(username) +
        "\",\"password\":\"" +
        json_escape(password) +
        "\"}";

    return send_api_request(body);
}

bool login_account(const char* username, const char* password)
{
    std::string body =
        "{\"cmd\":\"LOGINACC\",\"username\":\"" +
        json_escape(username) +
        "\",\"password\":\"" +
        json_escape(password) +
        "\"}";

    return send_api_request(body);
}

bool send_chat(const char* username, const char* password, const char* message)
{
    std::string body =
        "{\"cmd\":\"CHAT\",\"content\":\"" +
        json_escape(message) +
        "\",\"username\":\"" +
        json_escape(username) +
        "\",\"password\":\"" +
        json_escape(password) +
        "\"}";

    return send_api_request(body);
}
