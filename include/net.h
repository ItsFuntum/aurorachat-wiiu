#pragma once
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cstddef>

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
#define SERVER_PORT 8961

int ConnectToServer();
void TryReceive(int* sock);
void send_chat_line(int* sock, const char* username, const char* input);
