/**
 * TCP Server - 网络协议综合实验
 *
 * 功能：监听端口，接受多个客户端连接，进行双向聊天
 * 平台：Windows（使用 WinSock2）
 * 特点：每连接一个客户端，创建一个独立线程处理（支持并发）
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
#define DEFAULT_PORT 6000     // 监听端口号
#define MAX_BUFFER_SIZE 1024  // 消息缓冲区大小

// ========== 函数声明 ==========
static unsigned __stdcall handle_client(void *arg); // 客户端处理线程函数
static void send_greeting(SOCKET c);                // 向新连接发送欢迎消息
static int init_winsock(void);                      // 初始化 WinSock 库
static SOCKET create_socket(void);                  // 创建 TCP socket
static int bind_port(SOCKET s, int port);           // 将 socket 绑定到端口
static int start_listen(SOCKET s);                  // 开始监听连接请求

// ========== 主函数 ==========
int main(void)
{
    // 打印程序标题
    printf("======================================\n");
    printf("     TCP Server - Network Experiment  \n");
    printf("======================================\n");
    printf("Listening port: %d\n\n", DEFAULT_PORT);

    // 初始化 WinSock，失败则退出
    if (init_winsock() != 0)
        return 1;

    // 创建服务器 socket（监听 socket）
    SOCKET sock_srv = create_socket();
    if (sock_srv == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    // 将 socket 绑定到指定端口
    // 绑定后，操作系统会将该端口的网络数据交给此 socket 处理
    if (bind_port(sock_srv, DEFAULT_PORT) != 0) {
        closesocket(sock_srv);
        WSACleanup();
        return 1;
    }

    // 开始监听，backlog = 5 表示等待队列最大长度
    if (start_listen(sock_srv) != 0) {
        closesocket(sock_srv);
        WSACleanup();
        return 1;
    }

    printf("[Server] Waiting for connection...\n");

    // ========== 主循环：接受客户端连接 ==========
    //
    // 服务器进程通常是一个无限循环，持续等待客户端的连接请求
    // 每当一个客户端连接，就创建一个线程去处理它，然后继续等待下一个
    //
    // 这是一种经典的"并发服务器"模型（每个客户一个线程）
    while (1) {
        struct sockaddr_in addr_client;  // 存储客户端地址信息
        int len = sizeof(struct sockaddr_in);

        // accept() 从等待队列中取出一个已完成 TCP 三次握手的连接
        //
        // 重要概念——三次握手（TCP three-way handshake）：
        //   1. 客户端发送 SYN（连接请求）
        //   2. 服务器返回 SYN+ACK（确认并同意连接）
        //   3. 客户端发送 ACK（确认）
        //   → 此时连接建立完成，accept() 返回新的 socket
        //
        // 如果队列为空，accept() 会阻塞（等待直到有客户端连接）
        //
        // 返回值：新的连接 socket（与监听 socket 不同，用于通信）
        //         INVALID_SOCKET = 出错
        SOCKET sock_conn = accept(sock_srv, (struct sockaddr *)&addr_client, &len);
        if (sock_conn == INVALID_SOCKET) {
            printf("[Error] accept failed: %d\n", WSAGetLastError());
            continue; // 出错后继续等待下一个连接
        }

        // 打印客户端信息
        // inet_ntoa：将网络字节序的 IP 转换为点分十进制字符串
        // ntohs：将网络字节序的端口号转换为主机字节序
        printf("[Server] Client connected from: %s:%d\n",
               inet_ntoa(addr_client.sin_addr),
               ntohs(addr_client.sin_port));

        // 连接刚建立，立即发送欢迎消息
        send_greeting(sock_conn);

        // 创建独立线程处理该客户端
        // 注意：服务器端为主动端（服务员），这里是"接待员"——只负责接待，
        // 具体服务交给专职线程（handle_client）去做
        // 这样服务器可以同时服务多个客户端，互不阻塞
        HANDLE thread = (HANDLE)_beginthreadex(
            NULL, 0, handle_client,
            (void *)sock_conn,  // 线程参数：传递连接 socket
            0, NULL);

        if (thread == NULL) {
            // 线程创建失败（系统资源耗尽），关闭连接
            printf("[Error] Failed to create thread.\n");
            closesocket(sock_conn);
        } else {
            CloseHandle(thread); // 线程已启动，立即关闭句柄（线程独立运行）
        }
    }

    // （此行永远不会执行，因为主循环是无限循环）
    // 如果要优雅退出，需在收到特定信号后 break
    closesocket(sock_srv);
    WSACleanup();
    return 0;
}

// ========== 客户端处理线程函数 ==========
//
// 每个客户端连接都会创建一个独立的线程运行此函数
// 实现双向通信：
//   - 接收客户端消息并回复（服务器被动响应）
//   - 服务器用户可通过键盘输入主动发消息
//
// 使用 select() 实现非阻塞 I/O，避免在 recv() 和键盘输入上阻塞
static unsigned __stdcall handle_client(void *arg)
{
    // arg 是主线程传递过来的连接 socket
    SOCKET sock_conn = (SOCKET)arg;
    char recv_buf[MAX_BUFFER_SIZE];   // 接收客户端数据的缓冲区
    char input_buf[MAX_BUFFER_SIZE];  // 服务器用户输入的缓冲区
    int connected = 1;                // 本线程的连接状态标志

    // 线程主循环
    while (connected) {

        // ---- 第一部分：检查并接收客户端发来的数据 ----

        // 使用 select() 实现非阻塞检查
        // select 可以同时监听多个"文件描述符"的可读性，这里只监听 sock_conn 一个 socket
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);           // 清空文件描述符集合
        FD_SET(sock_conn, &readfds); // 将客户端 socket 加入集合
        tv.tv_sec = 0;               // 超时：0 秒
        tv.tv_usec = 100000;         // 超时：100000 微秒 = 0.1 秒

        // select() 返回值：
        //   > 0：有 socket 可读（数据已到达）
        //   = 0：超时，没有数据
        //   < 0：出错
        int ret = select(0, &readfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(sock_conn, &readfds)) {
            // 确认 sock_conn 有数据可读

            memset(recv_buf, 0, sizeof(recv_buf)); // 清空缓冲区

            // recv() 从 socket 读取数据
            // 参数：
            //   - sock_conn：连接 socket
            //   - recv_buf：接收缓冲区
            //   - MAX_BUFFER_SIZE - 1：最大读取字节数（留一个位置给 '\0'）
            //   - 0：标志位（默认）
            //
            // 返回值：
            //   - > 0：实际接收的字节数
            //   - = 0：客户端主动关闭了连接
            //   - < 0：出错了
            int n = recv(sock_conn, recv_buf, MAX_BUFFER_SIZE - 1, 0);

            if (n > 0) {
                // 成功收到数据
                recv_buf[n] = '\0'; // 添加字符串结束符（recv 不自动添加）
                printf("[Server] Client: %s", recv_buf);

                // 检查客户端是否发送了 quit 命令
                if (strcmp(recv_buf, "quit\r\n") == 0 ||
                    strcmp(recv_buf, "quit") == 0) {
                    const char *bye = "Goodbye!\r\n";
                    send(sock_conn, bye, (int)strlen(bye), 0);
                    printf("[Server] Client requested to quit.\n");
                    connected = 0; // 退出循环
                } else {
                    // 服务器回复 ACK（确认收到）
                    // snprintf 安全地格式化字符串，避免缓冲区溢出
                    char reply[MAX_BUFFER_SIZE];
                    snprintf(reply, sizeof(reply), "[Server] ACK: %s", recv_buf);
                    send(sock_conn, reply, (int)strlen(reply), 0);
                }
            } else {
                // recv 返回 0 表示客户端已断开连接
                printf("[Server] Client disconnected.\n");
                connected = 0; // 退出循环
            }
        }

        // ---- 第二部分：检查服务器用户的键盘输入 ----

        // _kbhit()：检测是否有键盘按键（不阻塞，无输入时立即返回）
        // 这允许服务器用户在不影响接收的情况下主动发送消息
        if (_kbhit()) {
            memset(input_buf, 0, sizeof(input_buf));
            fgets(input_buf, sizeof(input_buf), stdin);

            if (strlen(input_buf) > 0) {
                // 将用户输入发送给客户端
                int n = send(sock_conn, input_buf, (int)strlen(input_buf), 0);
                if (n > 0) {
                    printf("[Server] Sent: %s", input_buf);
                }

                // 如果服务器用户输入 quit，主动关闭连接
                if (strcmp(input_buf, "quit\r\n") == 0 ||
                    strcmp(input_buf, "quit") == 0) {
                    connected = 0;
                }
            }
        }
        // 循环每次最多阻塞 0.1 秒（select 超时），然后继续检查
    }

    // 线程结束，关闭连接 socket
    closesocket(sock_conn);

    // 释放线程资源（Windows C 运行时库要求，类似于 pthread_exit）
    _endthreadex(0);
    return 0;
}

// ========== 发送欢迎消息 ==========
// 在 accept() 返回后立即调用，建立良好的首次交互体验
static void send_greeting(SOCKET c)
{
    const char *greeting = "Welcome! You are connected to TCP Server.\r\n";
    // send() 将数据写入 socket 的发送缓冲区
    // 数据会通过网络传输到对方主机的对应 socket
    send(c, greeting, (int)strlen(greeting), 0);
}

// ========== 初始化 WinSock 库 ==========
// WinSock（Windows Socket）是 Windows 的网络编程 API
// 使用前必须调用 WSAStartup() 初始化，程序结束时调用 WSACleanup() 卸载
static int init_winsock(void)
{
    WSADATA wsa_data;

    // MAKEWORD(2, 2)：指定版本号 2.2
    // WinSock 2.2 是目前最广泛使用的版本，支持更多功能如重叠 I/O
    WORD version = MAKEWORD(2, 2);

    // WSAStartup() 进行初始化
    // 返回值：0=成功，否则表示初始化失败
    if (WSAStartup(version, &wsa_data) != 0) {
        printf("[Error] WSAStartup failed.\n");
        return -1;
    }
    return 0;
}

// ========== 创建 TCP Socket ==========
//
// socket() 创建一个新的套接字（file descriptor）
//
// 参数详解：
//   - AF_INET：地址族（Address Family），表示 IPv4 网络
//     （还有 AF_INET6 = IPv6，AF_UNIX = 本地进程通信）
//   - SOCK_STREAM：套接字类型，表示面向流的协议（TCP）
//     （SOCK_DGRAM = 数据报协议 = UDP，无连接、不可靠）
//   - 0：协议类型。设为 0 时，系统根据前两个参数自动选择
//        AF_INET + SOCK_STREAM → 自动选择 TCP
//        AF_INET + SOCK_DGRAM  → 自动选择 UDP
//
// 返回值：
//   - 成功：返回一个新的 socket 描述符（正整数）
//   - 失败：返回 INVALID_SOCKET（强制转换为 -1）
static SOCKET create_socket(void)
{
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET)
        printf("[Error] socket creation failed: %d\n", WSAGetLastError());
    return s;
}

// ========== 绑定端口 ==========
// 将 socket 与一个具体的 IP 地址和端口号关联起来
static int bind_port(SOCKET s, int port)
{
    struct sockaddr_in addr; // IPv4 地址结构体

    // sockaddr_in 结构体成员：
    //   - sin_family：地址族，必须与 socket() 创建时一致
    //   - sin_port：端口号（16位无符号整数）
    //   - sin_addr：IP 地址
    //
    // 字节序转换（非常重要！）：
    //   - 主机字节序（Host Byte Order）：本地 CPU 使用的字节顺序
    //     （x86/x64 是小端序，低字节在前）
    //   - 网络字节序（Network Byte Order）：网络协议统一规定为大端序
    //
    // 函数说明：
    //   - htons()：Host to Network Short（16位，端口号用）
    //   - htonl()：Host to Network Long（32位，IP 用）
    //   - ntohs()：Network to Host Short
    //   - ntohl()：Network to Host Long
    addr.sin_family = AF_INET;                       // IPv4
    addr.sin_port = htons((unsigned short)port);     // 端口号：主机序→网络序
    addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);  // IP：INADDR_ANY = 0.0.0.0
                                                     // 表示监听所有网卡的该端口
                                                     // （能接收来自任何网卡的连接）

    // bind() 将 socket 绑定到地址
    // 绑定后，该 socket 与指定的 IP:端口 关联
    // 操作系统会把发往该地址的数据送到这个 socket
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("[Error] bind failed: %d\n", WSAGetLastError());
        return -1;
    }
    return 0;
}

// ========== 开始监听 ==========
// listen() 将 socket 转换为监听状态，允许客户端发起连接
static int start_listen(SOCKET s)
{
    // listen() 将 socket 从主动连接 socket 转为监听 socket
    // 参数：
    //   - s：已绑定端口的 socket
    //   - 5：backlog，等待队列的最大长度
    //
    // 等待队列解释：
    //   TCP 三次握手需要时间，当服务器忙于处理其他连接时，
    //   新连接会放入 backlog 队列等待
    //   如果队列满，新连接会被拒绝（客户端收到错误）
    if (listen(s, 5) == SOCKET_ERROR) {
        printf("[Error] listen failed: %d\n", WSAGetLastError());
        return -1;
    }
    return 0;
}