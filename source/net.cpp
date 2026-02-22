#include "net.h"
#include "chat.h"

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

void ReconnectToTCPServer()
{
    AddChatLine(tvRenderer, "[LOG]: Reconnecting...", fontSize, tvTextColor, maxWidth);

    int newSock = ConnectToTCPServer();

    if (newSock >= 0) {
        sock = newSock;
        connectionLost = false;

        AddChatLine(tvRenderer, "[LOG]: Successfully reconnected!", fontSize, tvTextColor, maxWidth);
    }
    else {
        AddChatLine(tvRenderer, "[LOG]: Reconnect failed. Please check your internet connection and try again. The server may also be down at the moment. Thank you for your patience.", fontSize, tvTextColor, maxWidth);
    }
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

void TryReceive(int *sock, SDL_Renderer* renderer, int fontSize, SDL_Color textColor, int maxWidth)
{
    if (*sock < 0) return;

    static std::string pending;
    char buf[512];

    while (true)
    {
        ssize_t r = recv(*sock, buf, sizeof(buf), 0);

        if (r > 0)
        {
            pending.append(buf, r);

            size_t pos;
            while ((pos = pending.find('\n')) != std::string::npos)
            {
                std::string line = pending.substr(0, pos);
                pending.erase(0, pos + 1);

                AddChatLine(renderer, line.c_str(), fontSize, textColor, maxWidth);
            }
        }
        else if (r == 0)
        {
            AddChatLine(renderer, "[ERROR]: Disconnected from server. Please reconnect to the server or restart your client.", fontSize, textColor, maxWidth);

            close(*sock);
            *sock = -1;
            pending.clear();

            connectionLost = true;
            break;
        }
        else
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                break;

            AddChatLine(renderer, "[ERROR]: An error occurred while receiving data. Please reconnect to the server or restart your client.", fontSize, textColor, maxWidth);

            close(*sock);
            *sock = -1;
            pending.clear();

            connectionLost = true;
            break;
        }
    }
}

std::string send_api_request(const std::string& jsonBody)
{
    int sock = ConnectToHTTPServer();
    if (sock < 0) return "";

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

    // ---- Send ----
    int totalSent = 0;
    while (totalSent < reqLen) {
        int sent = send(sock, request + totalSent, reqLen - totalSent, 0);
        if (sent <= 0) {
            close(sock);
            return "";
        }
        totalSent += sent;
    }

    // ---- Read response ----
    std::string response;
    char buffer[1024];

    int r;
    while ((r = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[r] = '\0';
        response += buffer;
    }

    close(sock);
    return response;
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

std::string connect_to_api()
{
    std::string body = "{\"cmd\":\"CONNECT\",\"version\":\"" +
    clientVersion +
    "\",\"platform\":\"Wii U\"}";
    return send_api_request(body);
}

std::string make_account(const char* username, const char* password)
{
    std::string body =
        "{\"cmd\":\"MAKEACC\",\"username\":\"" +
        json_escape(username) +
        "\",\"password\":\"" +
        json_escape(password) +
        "\"}";

    return send_api_request(body);
}

std::string login_account(const char* username, const char* password)
{
    std::string body =
        "{\"cmd\":\"LOGINACC\",\"username\":\"" +
        json_escape(username) +
        "\",\"password\":\"" +
        json_escape(password) +
        "\"}";

    return send_api_request(body);
}

std::string send_chat(const char* username, const char* password, const char* message)
{
    std::string body =
        "{\"cmd\":\"CHAT\",\"content\":\"" +
        json_escape(message) +
        "\",\"username\":\"" +
        json_escape(username) +
        "\",\"password\":\"" +
        json_escape(password) +
        "\",\"platform\":\"Wii U\"}";

    return send_api_request(body);
}
