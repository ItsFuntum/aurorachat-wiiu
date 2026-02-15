#pragma once
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cstddef>
#include <string>

#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "net.h"
#include "chat.h"

#define SERVER_IP "104.236.25.60"
#define SERVER_PORT_TCP 4040
#define SERVER_PORT_HTTP 3072

int ConnectToTCPServer();
int ConnectToHTTPServer();

void TryReceive(int *sock, SDL_Renderer* renderer, int fontSize, SDL_Color textColor, int maxWidth);
std::string send_api_request(const std::string& jsonBody);

std::string json_escape(const char* input);

std::string make_account(const char* username, const char* password);
std::string login_account(const char* username, const char* password);
std::string send_chat(const char* username, const char* password, const char* message);
