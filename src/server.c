#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 6000
#define MAX_BUFFER_SIZE 1024

static unsigned __stdcall handle_client(void *arg);
static void send_greeting(SOCKET c);
static int init_winsock(void);
static SOCKET create_socket(void);
static int bind_port(SOCKET s, int port);
static int start_listen(SOCKET s);

int main(void)
{
    printf("======================================\n");
    printf("     TCP Server - Network Experiment  \n");
    printf("======================================\n");
    printf("Listening port: %d\n\n", DEFAULT_PORT);

    if (init_winsock() != 0)
        return 1;

    SOCKET sock_srv = create_socket();
    if (sock_srv == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    if (bind_port(sock_srv, DEFAULT_PORT) != 0) {
        closesocket(sock_srv);
        WSACleanup();
        return 1;
    }

    if (start_listen(sock_srv) != 0) {
        closesocket(sock_srv);
        WSACleanup();
        return 1;
    }

    printf("[Server] Waiting for connection...\n");

    while (1) {
        struct sockaddr_in addr_client;
        int len = sizeof(struct sockaddr_in);

        SOCKET sock_conn = accept(sock_srv, (struct sockaddr *)&addr_client, &len);
        if (sock_conn == INVALID_SOCKET) {
            printf("[Error] accept failed: %d\n", WSAGetLastError());
            continue;
        }

        printf("[Server] Client connected from: %s:%d\n",
               inet_ntoa(addr_client.sin_addr),
               ntohs(addr_client.sin_port));

        send_greeting(sock_conn);

        /* Create a thread to handle this client */
        HANDLE thread = (HANDLE)_beginthreadex(
            NULL, 0, handle_client,
            (void *)sock_conn,
            0, NULL);

        if (thread == NULL) {
            printf("[Error] Failed to create thread.\n");
            closesocket(sock_conn);
        } else {
            CloseHandle(thread);
        }
    }

    closesocket(sock_srv);
    WSACleanup();
    return 0;
}

/* Thread function: bidirectional chat */
static unsigned __stdcall handle_client(void *arg)
{
    SOCKET sock_conn = (SOCKET)arg;
    char recv_buf[MAX_BUFFER_SIZE];
    char input_buf[MAX_BUFFER_SIZE];
    int connected = 1;

    while (connected) {
        /* Non-blocking check if there's data to receive */
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(sock_conn, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000; /* 0.1 second */

        int ret = select(0, &readfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(sock_conn, &readfds)) {
            memset(recv_buf, 0, sizeof(recv_buf));
            int n = recv(sock_conn, recv_buf, MAX_BUFFER_SIZE - 1, 0);

            if (n > 0) {
                recv_buf[n] = '\0';
                printf("[Server] Client: %s", recv_buf);

                if (strcmp(recv_buf, "quit\r\n") == 0 ||
                    strcmp(recv_buf, "quit") == 0) {
                    const char *bye = "Goodbye!\r\n";
                    send(sock_conn, bye, (int)strlen(bye), 0);
                    printf("[Server] Client requested to quit.\n");
                    connected = 0;
                } else {
                    char reply[MAX_BUFFER_SIZE];
                    snprintf(reply, sizeof(reply), "[Server] ACK: %s", recv_buf);
                    send(sock_conn, reply, (int)strlen(reply), 0);
                }
            } else {
                printf("[Server] Client disconnected.\n");
                connected = 0;
            }
        }

        /* Check if server user wants to send a message */
        if (_kbhit()) {
            memset(input_buf, 0, sizeof(input_buf));
            fgets(input_buf, sizeof(input_buf), stdin);

            if (strlen(input_buf) > 0) {
                int n = send(sock_conn, input_buf, (int)strlen(input_buf), 0);
                if (n > 0) {
                    printf("[Server] Sent: %s", input_buf);
                }

                if (strcmp(input_buf, "quit\r\n") == 0 ||
                    strcmp(input_buf, "quit") == 0) {
                    connected = 0;
                }
            }
        }
    }

    closesocket(sock_conn);
    _endthreadex(0);
    return 0;
}

static void send_greeting(SOCKET c)
{
    const char *greeting = "Welcome! You are connected to TCP Server.\r\n";
    send(c, greeting, (int)strlen(greeting), 0);
}

static int init_winsock(void)
{
    WSADATA wsa_data;
    WORD version = MAKEWORD(2, 2);
    if (WSAStartup(version, &wsa_data) != 0) {
        printf("[Error] WSAStartup failed.\n");
        return -1;
    }
    return 0;
}

static SOCKET create_socket(void)
{
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET)
        printf("[Error] socket creation failed: %d\n", WSAGetLastError());
    return s;
}

static int bind_port(SOCKET s, int port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("[Error] bind failed: %d\n", WSAGetLastError());
        return -1;
    }
    return 0;
}

static int start_listen(SOCKET s)
{
    if (listen(s, 5) == SOCKET_ERROR) {
        printf("[Error] listen failed: %d\n", WSAGetLastError());
        return -1;
    }
    return 0;
}