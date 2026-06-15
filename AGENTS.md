# AGENTS.md - Claude Code Agent 索引

本文件为 Claude Code 提供项目上下文和标准实现参考。

---

## 代码风格

- **缩进**: 4 空格（Tab），不混用 Tab 和空格
- **函数命名**: 动词_名词，下划线分隔，全部小写，如 `init_winsock`, `create_socket`
- **变量命名**: 小写下划线，如 `sock_srv`, `send_buf`, `recv_len`
- **常量命名**: 全大写下划线，如 `DEFAULT_PORT`, `MAX_BUFFER_SIZE`
- **Winsock 头文件**: `#include <winsock2.h>`
- **链接库**: `#pragma comment(lib, "ws2_32.lib")` 或编译参数 `-lws2_32`
- **错误处理**: 每次 Winsock 调用必须检查返回值

---

## 标准实现步骤（TCP 服务器）

```
1. WSAStartup 初始化
   WSADATA wsa_data;
   WSAStartup(MAKEWORD(2, 2), &wsa_data);

2. socket 创建
   SOCKET sock_srv = socket(AF_INET, SOCK_STREAM, 0);

3. bind 绑定端口
   struct sockaddr_in addr_srv;
   addr_srv.sin_family = AF_INET;
   addr_srv.sin_port = htons(6000);
   addr_srv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
   bind(sock_srv, (SOCKADDR*)&addr_srv, sizeof(SOCKADDR));

4. listen 监听
   listen(sock_srv, 5);

5. accept 接受连接（循环）
   struct sockaddr_in addr_client;
   int len = sizeof(SOCKADDR);
   SOCKET sock_conn = accept(sock_srv, (SOCKADDR*)&addr_client, &len);

6. send/recv 消息循环
   char recv_buf[1024];
   int recv_len = recv(sock_conn, recv_buf, sizeof(recv_buf), 0);
   if (strcmp(recv_buf, "quit") == 0) break;
   send(sock_conn, "ACK", 4, 0);

7. closesocket + WSACleanup
   closesocket(sock_conn);
   closesocket(sock_srv);
   WSACleanup();
```

---

## 标准实现步骤（TCP 客户端）

```
1. WSAStartup 初始化

2. socket 创建
   SOCKET sock_client = socket(AF_INET, SOCK_STREAM, 0);

3. connect 连接服务器
   struct sockaddr_in addr_srv;
   addr_srv.sin_family = AF_INET;
   addr_srv.sin_port = htons(6000);
   addr_srv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
   connect(sock_client, (SOCKADDR*)&addr_srv, sizeof(SOCKADDR));

4. recv/send 消息循环

5. closesocket + WSACleanup
```

---

## 关键代码片段

### 字节序转换

```c
htonl(INADDR_ANY)      // host to network long，本机字节序转网络字节序
htons(6000)            // host to network short
inet_addr("127.0.0.1") // 点分十进制 IP 转网络字节序
inet_ntoa(addr.sin_addr) // 网络字节序转点分十进制字符串
```

### recv 接收一行数据

```c
char recv_buf[1024];
int n = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
if (n > 0) {
    recv_buf[n] = '\0';
    printf("Received: %s\n", recv_buf);
}
```

### send 发送字符串

```c
const char* msg = "Hello from server!";
send(sock, msg, strlen(msg), 0);
```

---

## 调试方法

### 服务器调试

在 accept、send、recv 关键位置添加 printf 跟踪：

```c
printf("[Server] Waiting for connection on port %d...\n", port);
printf("[Server] Client connected from: %s:%d\n",
       inet_ntoa(addr_client.sin_addr), ntohs(addr_client.sin_port));
printf("[Server] Received (%d bytes): %s\n", recv_len, recv_buf);
printf("[Server] Sent %d bytes to client\n", send_len);
```

### 客户端调试

```c
printf("[Client] Connecting to %s:%d...\n", ip, port);
printf("[Client] Connected successfully!\n");
printf("[Client] Received: %s\n", recv_buf);
```

---

## 文件索引

| 文件 | 职责 |
|------|------|
| `src/server.c` | TCP 服务器主程序 |
| `src/client.c` | TCP 客户端主程序 |
| `docs/requirements.md` | 原始实验需求 |
| `docs/plan.md` | 实施计划 |

---

## 常见问题

**Q: connect 返回 -1**
A: 确认服务器已启动并监听，检查 IP 和端口是否正确，防火墙是否放行。

**Q: recv 返回 0**
A: 对端已正常关闭连接。

**Q: 中文字符串显示乱码**
A: 源文件保存为 UTF-8 无 BOM，字符串使用英文避免乱码，或在 Windows 控制台用 `chcp 65001` 切换编码。

**Q: 编译报错 undefined reference to `socket` 等**
A: 确认链接了 ws2_32 库，MinGW 用 `-lws2_32`，MSVC 自动链接。