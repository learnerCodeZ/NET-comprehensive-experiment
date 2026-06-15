#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 6000
#define DEFAULT_IP   "127.0.0.1"
#define MAX_BUFFER_SIZE 1024

static SOCKET g_sock = INVALID_SOCKET;
static int g_connected = 1;

static unsigned __stdcall recv_thread(void *arg);
static int init_winsock(void);
static SOCKET create_socket(void);
static int connect_server(SOCKET s, const char *ip, int port);
static void show_menu(void);
static int get_choice(void);
static void cleanup(void);

int main(void)
{
    printf("======================================\n");
    printf("     TCP Client - Network Experiment  \n");
    printf("======================================\n");
    printf("Listening port: %d\n\n", DEFAULT_PORT);

    if (init_winsock() != 0)
        return 1;

    show_menu();

    while (1) {
        int choice = get_choice();

        switch (choice) {
            case 1: {
                if (g_sock != INVALID_SOCKET) {
                    printf("[Client] Already connected.\n");
                    break;
                }
                g_sock = create_socket();
                if (g_sock == INVALID_SOCKET)
                    break;

                if (connect_server(g_sock, DEFAULT_IP, DEFAULT_PORT) != 0) {
                    closesocket(g_sock);
                    g_sock = INVALID_SOCKET;
                    break;
                }

                g_connected = 1;
                printf("[Client] Connected successfully!\n");
                printf("Welcome! You are connected to TCP Server.\n\n");

                /* Start recv thread */
                HANDLE thread = (HANDLE)_beginthreadex(
                    NULL, 0, recv_thread, NULL, 0, NULL);
                if (thread)
                    CloseHandle(thread);

                break;
            }
            case 2: {
                if (g_sock == INVALID_SOCKET) {
                    printf("[Client] Not connected. Choose 1 first.\n\n");
                    break;
                }
                printf("You: ");
                char msg[MAX_BUFFER_SIZE];
                if (fgets(msg, sizeof(msg), stdin) == NULL)
                    break;

                send(g_sock, msg, (int)strlen(msg), 0);

                if (strcmp(msg, "quit\r\n") == 0 ||
                    strcmp(msg, "quit") == 0) {
                    printf("\n[Client] Disconnecting...\n");
                    g_connected = 0;
                    closesocket(g_sock);
                    g_sock = INVALID_SOCKET;
                }
                break;
            }
            case 3: {
                if (g_sock == INVALID_SOCKET) {
                    printf("[Client] Not connected.\n\n");
                    break;
                }
                send(g_sock, "quit", 4, 0);
                g_connected = 0;
                closesocket(g_sock);
                g_sock = INVALID_SOCKET;
                printf("[Client] Disconnected.\n\n");
                break;
            }
            case 4: {
                cleanup();
                printf("[Client] Goodbye!\n");
                return 0;
            }
            default:
                printf("[Error] Invalid choice.\n\n");
        }
    }
}

static unsigned __stdcall recv_thread(void *arg)
{
    char recv_buf[MAX_BUFFER_SIZE];
    (void)arg;

    while (g_connected) {
        if (g_sock == INVALID_SOCKET)
            break;

        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(g_sock, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int ret = select(0, &readfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(g_sock, &readfds)) {
            memset(recv_buf, 0, sizeof(recv_buf));
            int n = recv(g_sock, recv_buf, MAX_BUFFER_SIZE - 1, 0);

            if (n > 0) {
                recv_buf[n] = '\0';
                printf("\n[Server] %s", recv_buf);
                printf("\nYou: ");
                fflush(stdout);
            } else {
                printf("\n[Server] Disconnected.\n\n");
                g_connected = 0;
                break;
            }
        }
    }

    _endthreadex(0);
    return 0;
}

static void show_menu(void)
{
    printf("  1. Connect to server\n");
    printf("  2. Send message\n");
    printf("  3. Disconnect\n");
    printf("  4. Quit\n");
    printf("======================================\n");
    printf("Choice: ");
}

static int get_choice(void)
{
    int choice;
    if (scanf("%d", &choice) != 1) {
        while (getchar() != '\n');
        return -1;
    }
    while (getchar() != '\n');
    return choice;
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

static int connect_server(SOCKET s, const char *ip, int port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.S_un.S_addr = inet_addr(ip);

    printf("[Client] Connecting to %s:%d...\n", ip, port);
    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("[Error] connect failed: %d\n", WSAGetLastError());
        return -1;
    }
    return 0;
}

static void cleanup(void)
{
    if (g_sock != INVALID_SOCKET) {
        g_connected = 0;
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
    }
    WSACleanup();
}