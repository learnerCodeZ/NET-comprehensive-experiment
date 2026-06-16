/**
 * TCP Client - 网络协议综合实验
 *
 * 功能：连接 TCP 服务器，进行双向聊天通信
 * 平台：Windows（使用 WinSock2）
 */

// ========== 头文件 ==========
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>
#include <conio.h>

// 链接 WinSock2 库（Windows 平台需要）
#pragma comment(lib, "ws2_32.lib")

// ========== 宏定义 ==========
#define DEFAULT_PORT 6000        // 默认服务器端口号
#define DEFAULT_IP   "127.0.0.1" // 默认服务器 IP（本地回环地址）
#define MAX_BUFFER_SIZE 1024     // 消息缓冲区大小

// ========== 全局变量 ==========
static SOCKET g_sock = INVALID_SOCKET; // 全局客户端 socket（INVALID_SOCKET = 无效套接字）
static int g_connected = 1;            // 连接状态标志：1=已连接，0=已断开

// ========== 函数声明 ==========
static unsigned __stdcall recv_thread(void *arg);   // 接收消息线程函数
static int init_winsock(void);                      // 初始化 WinSock 库
static SOCKET create_socket(void);                  // 创建 TCP socket
static int connect_server(SOCKET s, const char *ip, int port); // 连接到服务器
static void show_menu(void);                        // 显示菜单
static int get_choice(void);                        // 获取用户输入的选项
static void cleanup(void);                          // 清理资源（关闭 socket、WSA）

// ========== 主函数 ==========
int main(void)
{
    // 打印程序标题
    printf("======================================\n");
    printf("     TCP Client - Network Experiment  \n");
    printf("======================================\n");
    printf("Listening port: %d\n\n", DEFAULT_PORT);

    // 初始化 WinSock（Windows 套接字库），失败则退出
    if (init_winsock() != 0)
        return 1;

    // 显示交互菜单
    show_menu();

    // 主循环：持续等待用户输入
    while (1) {
        int choice = get_choice();

        switch (choice) {
            case 1: {
                // ---- 选项 1：连接服务器 ----

                // 检查是否已连接，防止重复连接
                if (g_sock != INVALID_SOCKET) {
                    printf("[Client] Already connected.\n");
                    break;
                }

                // 创建 socket
                g_sock = create_socket();
                if (g_sock == INVALID_SOCKET)
                    break; // 创建失败

                // 尝试连接服务器
                if (connect_server(g_sock, DEFAULT_IP, DEFAULT_PORT) != 0) {
                    // 连接失败，关闭 socket 并重置为无效
                    closesocket(g_sock);
                    g_sock = INVALID_SOCKET;
                    break;
                }

                // 连接成功
                g_connected = 1;
                printf("[Client] Connected successfully!\n");
                printf("Welcome! You are connected to TCP Server.\n\n");

                // 启动独立线程接收服务器消息
                // _beginthreadex 是 Windows 的多线程创建函数
                // 参数：安全属性、栈大小、线程函数、参数、创建标志、线程ID
                // 线程函数 recv_thread 会在后台运行，持续监听服务器发来的数据
                HANDLE thread = (HANDLE)_beginthreadex(
                    NULL, 0, recv_thread, NULL, 0, NULL);
                if (thread)
                    CloseHandle(thread); // 立即关闭线程句柄（线程已独立运行）

                break;
            }
            case 2: {
                // ---- 选项 2：发送消息 ----

                // 检查是否已连接，未连接则提示先选择 1
                if (g_sock == INVALID_SOCKET) {
                    printf("[Client] Not connected. Choose 1 first.\n\n");
                    break;
                }

                printf("You: ");

                // 从标准输入读取一行消息
                // fgets 会保留换行符 '\n'，recv 时服务器会收到带换行的内容
                char msg[MAX_BUFFER_SIZE];
                if (fgets(msg, sizeof(msg), stdin) == NULL)
                    break; // 读取失败

                // 通过 socket 发送数据到服务器
                // send 参数：socket、缓冲区、长度、标志（0=默认）
                send(g_sock, msg, (int)strlen(msg), 0);

                // 检查用户是否输入了 quit 命令（带换行和不带换行两种情况）
                if (strcmp(msg, "quit\r\n") == 0 ||
                    strcmp(msg, "quit") == 0) {
                    printf("\n[Client] Disconnecting...\n");
                    g_connected = 0;        // 通知接收线程退出
                    closesocket(g_sock);    // 关闭 socket
                    g_sock = INVALID_SOCKET;
                }
                break;
            }
            case 3: {
                // ---- 选项 3：主动断开连接 ----

                if (g_sock == INVALID_SOCKET) {
                    printf("[Client] Not connected.\n\n");
                    break;
                }

                // 向服务器发送 quit 命令通知对方
                send(g_sock, "quit", 4, 0);

                // 断开本地连接
                g_connected = 0;
                closesocket(g_sock);
                g_sock = INVALID_SOCKET;
                printf("[Client] Disconnected.\n\n");
                break;
            }
            case 4: {
                // ---- 选项 4：退出程序 ----
                cleanup();               // 清理所有资源
                printf("[Client] Goodbye!\n");
                return 0;                // 安全退出
            }
            default:
                printf("[Error] Invalid choice.\n\n");
        }
    }
}

// ========== 接收消息线程函数 ==========
// 该线程独立运行，持续检查 socket 是否有数据可读
// 使用 select() 实现非阻塞 I/O，每 0.1 秒检查一次
static unsigned __stdcall recv_thread(void *arg)
{
    char recv_buf[MAX_BUFFER_SIZE];
    (void)arg; // arg 是传入参数（本例中为 NULL），标记为未使用避免警告

    // 循环直到全局连接标志被设为 0
    while (g_connected) {

        // 检查 socket 是否有效
        if (g_sock == INVALID_SOCKET)
            break; // socket 已关闭，退出线程

        // fd_set：文件描述符集合（socket 在 Windows 中也是一种文件描述符）
        // select() 用于多路复用：同时监听多个 socket 的可读性
        // 这里只监听 g_sock 一个 socket
        fd_set readfds;
        struct timeval tv;

        FD_ZERO(&readfds);         // 清空集合
        FD_SET(g_sock, &readfds);  // 将 g_sock 加入待监听集合
        tv.tv_sec = 0;             // 等待超时：秒
        tv.tv_usec = 100000;       // 等待超时：微秒（100000us = 0.1秒）

        // select() 会阻塞直到：有数据到达 / 超时 / 出错
        // 参数：最大描述符数、读集合、写集合、异常集合、超时
        // 返回值：>0 表示有 socket 可读，0=超时，-1=出错
        int ret = select(0, &readfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(g_sock, &readfds)) {
            // 确定 g_sock 有数据可读

            memset(recv_buf, 0, sizeof(recv_buf)); // 清空接收缓冲区

            // recv() 从 socket 接收数据
            // 参数：socket、缓冲区、最大长度、标志
            // 返回值：>0 实际接收字节数，0=对方关闭，-1=出错
            int n = recv(g_sock, recv_buf, MAX_BUFFER_SIZE - 1, 0);

            if (n > 0) {
                // 成功接收到数据
                recv_buf[n] = '\0';          // 添加字符串结束符（recv 不加）
                printf("\n[Server] %s", recv_buf); // 打印服务器消息
                printf("\nYou: ");           // 重新显示提示符
                fflush(stdout);              // 强制刷新输出（确保提示符立即显示）
            } else {
                // recv 返回 0 或 -1，表示服务器已断开连接
                printf("\n[Server] Disconnected.\n\n");
                g_connected = 0;  // 通知主循环
                break;            // 退出线程
            }
        }
        // 如果 select 超时（ret=0），继续循环检查，不阻塞
    }

    // 线程结束，释放线程资源（Windows C 运行时库要求）
    _endthreadex(0);
    return 0;
}

// ========== 显示菜单 ==========
static void show_menu(void)
{
    printf("  1. Connect to server\n");
    printf("  2. Send message\n");
    printf("  3. Disconnect\n");
    printf("  4. Quit\n");
    printf("======================================\n");
    printf("Choice: ");
}

// ========== 获取用户选择 ==========
// 返回用户输入的数字，失败返回 -1
static int get_choice(void)
{
    int choice;
    // scanf("%d") 尝试解析一个整数
    if (scanf("%d", &choice) != 1) {
        // 输入不是数字，清除输入缓冲区中的所有字符
        while (getchar() != '\n')
            ; // 逐个读取并丢弃直到换行符
        return -1;
    }
    // 读取并丢弃换行符，避免影响下次 fgets/scanf
    while (getchar() != '\n')
        ;
    return choice;
}

// ========== 初始化 WinSock 库 ==========
// WinSock（Windows Socket）是 Windows 的网络编程接口，类似于 Unix 的 BSD Socket
// 在使用任何 socket 函数之前，必须先调用 WSAStartup() 初始化库
static int init_winsock(void)
{
    WSADATA wsa_data;  // WSADATA 结构体，用于接收 WinSock 库的信息

    // MAKEWORD(2, 2) 构造版本号：主版本 2，次版本 2（WinSock 2.2）
    // WinSock 2.2 是主流版本，兼容性好
    WORD version = MAKEWORD(2, 2);

    // WSAStartup() 执行初始化
    // 返回值：0=成功，非0=失败
    if (WSAStartup(version, &wsa_data) != 0) {
        printf("[Error] WSAStartup failed.\n");
        return -1;
    }
    return 0;
}

// ========== 创建 TCP Socket ==========
// socket() 创建一个新的套接字
// 参数：
//   - AF_INET：地址族，IPv4（还有 AF_INET6 用于 IPv6）
//   - SOCK_STREAM：套接字类型，TCP（面向连接、可靠）
//   - 0：协议类型，由系统自动选择（SOCK_STREAM 对应 TCP）
// 返回：有效 socket 或 INVALID_SOCKET（失败）
static SOCKET create_socket(void)
{
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET)
        printf("[Error] socket creation failed: %d\n", WSAGetLastError());
    return s;
}

// ========== 连接到服务器 ==========
// 使用 TCP 三次握手与服务器建立连接
static int connect_server(SOCKET s, const char *ip, int port)
{
    // sockaddr_in：IPv4 地址结构体，包含：
    //   - sin_family：地址族（AF_INET）
    //   - sin_port：端口号（网络字节序）
    //   - sin_addr：IP 地址（网络字节序）
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;                            // IPv4
    addr.sin_port = htons((unsigned short)port);          // 主机字节序→网络字节序（16位）
    addr.sin_addr.S_un.S_addr = inet_addr(ip);           // 点分十进制 IP→网络字节序

    printf("[Client] Connecting to %s:%d...\n", ip, port);

    // connect() 向服务器发起连接请求
    // 如果服务器未运行，此调用会阻塞一段时间后返回错误
    // 成功返回 0，失败返回 SOCKET_ERROR
    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("[Error] connect failed: %d\n", WSAGetLastError());
        return -1;
    }
    return 0;
}

// ========== 清理资源 ==========
// 程序退出时调用，确保正确关闭所有 socket 和 WinSock 库
static void cleanup(void)
{
    if (g_sock != INVALID_SOCKET) {
        g_connected = 0;       // 先停止接收线程的循环
        closesocket(g_sock);   // 关闭 socket
        g_sock = INVALID_SOCKET;
    }
    WSACleanup();              // 卸载 WinSock 库，释放资源
}