# 实验报告内容

> 以下内容可直接复制粘贴到 Word 文档对应章节中。
> 图片（截图、流程图）需自行插入到 Word 中相应位置。

---

## 4.1 设计思路

### 1. 总体架构

本程序采用 **client/server（客户端/服务器）架构**，基于 TCP 协议实现网络通信。客户端与服务器分别独立运行，通过 socket 接口进行数据交换。

```
┌─────────────┐              TCP 连接              ┌─────────────┐
│   Client    │  ──────── connect() ─────────>    │   Server    │
│             │  <─────── send/recv ───────────>  │             │
│  菜单控制    │              quit                │  多线程处理  │
└─────────────┘                                  └─────────────┘
```

### 2. 为什么选择 TCP 协议

- TCP 提供可靠传输，保证消息有序、无丢失、无重复
- 交互式对话需要确认对方收到消息，TCP 的确认机制天然满足
- UDP 无连接、不保证送达，不适合本场景

### 3. 多线程设计

传统单线程方案只能"一方发、另一方收"，本程序采用多线程实现**双向同时对话**：

- **服务器**：每 `accept()` 一个客户端就创建一个线程，线程内 `select()` 检测网络数据 + `_kbhit()` 检测键盘输入，双方都能主动发消息
- **客户端**：主线程负责发送消息，`recv_thread()` 独立线程负责接收，双方互不阻塞

### 4. 模块化函数设计

将功能分解为多个独立函数，每个函数职责单一：

| 文件 | 函数 | 职责 |
|------|------|------|
| server.c | `init_winsock()` | Winsock 初始化 |
| server.c | `create_socket()` | 创建 TCP 套接字 |
| server.c | `bind_port()` | 绑定端口 6000 |
| server.c | `start_listen()` | 开始监听 |
| server.c | `handle_client()` 线程 | 双向消息处理 |
| client.c | `show_menu()` | 显示菜单 |
| client.c | `connect_server()` | 连接服务器 |
| client.c | `recv_thread()` 线程 | 独立接收消息 |

### 5. 端口设计

- 服务器监听端口：**6000**（实验指导单建议）
- 客户端连接地址：**127.0.0.1**（本地回环，同机测试）
- 客户端临时端口：操作系统自动分配（49152-65535 范围）

---

## 4.2 各函数流程图

### 服务器主流程（server.c main 函数）

```
开始
  │
  ▼
WSAStartup() 初始化 Winsock
  │
  ▼
socket() 创建 TCP 套接字
  │
  ▼
bind() 绑定 0.0.0.0:6000
  │
  ▼
listen() 开始监听
  │
  ▼
print("Waiting for connection...")
  │
  ▼
while(1) {
    accept() 接受客户端连接  ──> 显示客户端 IP
        │
        ▼
    send_greeting() 发送欢迎消息
        │
        ▼
    创建 handle_client 线程处理该客户端
    }
  │
  ▼
closesocket() + WSACleanup()
  │
  ▼
结束
```

### 服务器线程处理流程（handle_client 线程）

```
线程开始（传入 sock_conn）
  │
  ▼
while(connected) {
    select() 检测 socket 是否有数据
        │  有数据
        ▼
    recv() 接收消息
        │
        ▼
    strcmp(msg, "quit")?
        │ 是
        ▼
    发送 "Goodbye!"，connected=0，退出循环
        │否
        ▼
    发送 ACK 回复
    }
    │
    ▼
    _kbhit() 检测键盘是否有输入  ──>  有输入
        │                              ▼
        │                          fgets() 读取输入
        ▼                              │
    （继续循环）                     send() 发送给客户端
                                    strcmp(input, "quit")?
                                        │是
                                        ▼
                                    connected=0
}
  │
  ▼
closesocket(sock_conn)
  │
  ▼
线程结束
```

### 客户端主流程（client.c main 函数）

```
开始
  │
  ▼
WSAStartup() 初始化 Winsock
  │
  ▼
show_menu() 显示菜单
  │
  ▼
while(1) {
    get_choice() 读取用户选择
        │
        ├── 1: connect_server() ──> recv_greeting() ──> 创建 recv_thread 线程
        │
        ├── 2: fgets() 读取输入 ──> send() 发送
        │                       └─> 收到 ACK ──> 显示
        │
        ├── 3: send("quit") ──> closesocket()
        │
        └── 4: cleanup() ──> 退出程序
    }
}
```

### 客户端接收线程流程（recv_thread）

```
线程开始
  │
  ▼
while(g_connected) {
    select() 检测 socket 是否有数据
        │  有数据
        ▼
    recv() 接收消息
        │
        ▼
    print("[Server] %s")
    }
  │
  ▼
线程结束
```

---

## 4.3 核心代码分析

### 4.3.1 Winsock 初始化与清理

**服务器/客户端通用**（server.c 第 158-167 行，client.c 第 170-179 行）：

```c
// 初始化
WSADATA wsa_data;
WORD version = MAKEWORD(2, 2);       // 请求 Winsock 2.2 版本
WSAStartup(version, &wsa_data);      // 初始化，务必在所有 socket 操作前调用

// 清理（程序退出前）
WSACleanup();                         // 释放 Winsock 资源
```

> **说明**：`MAKEWORD(2, 2)` 表示请求 Winsock 2.2，是目前最常用的版本，兼容性好。

---

### 4.3.2 TCP 套接字创建

```c
// server.c 第 169 行 / client.c 第 189 行
SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
//         地址族    套接字类型        协议（0=自动选择TCP）
```

- `AF_INET`：IPv4 地址族
- `SOCK_STREAM`：字节流套接字 = **TCP 协议**（可靠、有序、面向连接）
- 若要 UDP：`SOCK_DGRAM`（无连接、不保证送达）

---

### 4.3.3 服务器绑定与监听

```c
// server.c 第 177-189 行
struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_port = htons(6000);                         // 端口 6000
addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);       // 接受任意网卡连接
bind(sock_srv, (struct sockaddr *)&addr, sizeof(addr));

// server.c 第 191-197 行
listen(sock_srv, 5);                                  // 队列长度 5
```

- `htonl(INADDR_ANY)`：绑定本机所有网络接口，局域网内其他设备也能访问
- `htons(6000)`：端口号字节序转换（网络统一用大端序）
- `listen()`：将主动套接字变为被动监听套接字，队列长度 5 表示最多排队 5 个连接

---

### 4.3.4 服务器接受连接

```c
// server.c 第 54 行
SOCKET sock_conn = accept(sock_srv, (struct sockaddr *)&addr_client, &len);

// 第 60-62 行：显示客户端 IP
printf("[Server] Client connected from: %s:%d\n",
       inet_ntoa(addr_client.sin_addr),           // IP 地址转换
       ntohs(addr_client.sin_port));              // 端口号转换
```

- `accept()`：阻塞等待，直到有客户端连接请求到达，返回新的连接套接字
- `inet_ntoa()`：将网络字节序的 IP 转为点分十进制字符串（如 `192.168.1.100`）
- 注意：`sock_srv` 始终监听，`sock_conn` 是与该客户端通信的专用套接字

---

### 4.3.5 客户端连接服务器

```c
// client.c 第 197-210 行
struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_port = htons(6000);
addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");  // 服务器地址
connect(sock_client, (struct sockaddr *)&addr, sizeof(addr));
```

- `inet_addr()`：将点分十进制 IP 转为网络字节序
- `connect()`：向服务器发起 TCP 连接，内部完成三次握手

---

### 4.3.6 多线程处理（核心亮点）

```c
// server.c 第 67-70 行：主线程每 accept 一个客户端就创建线程
HANDLE thread = _beginthreadex(
    NULL, 0,               // 安全属性、堆栈大小（默认）
    handle_client,         // 线程函数
    (void *)sock_conn,     // 传给线程的参数
    0, NULL);              // 创建标志、线程ID（不关心）
CloseHandle(thread);       // 立即关闭句柄，线程独立运行

// server.c 第 86-150 行：线程函数 handle_client() 同时收发
while (connected) {
    // 1. select 非阻塞检测网络数据
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock_conn, &readfds);
    tv.tv_usec = 100000;   // 0.1 秒超时
    int ret = select(0, &readfds, NULL, NULL, &tv);

    if (ret > 0 && FD_ISSET(sock_conn, &readfds)) {
        recv(sock_conn, recv_buf, MAX_BUFFER_SIZE, 0);
        // 处理消息...
    }

    // 2. _kbhit 检测键盘输入（服务器主动发消息）
    if (_kbhit()) {
        fgets(input_buf, sizeof(input_buf), stdin);
        send(sock_conn, input_buf, strlen(input_buf), 0);
    }
}
```

> **亮点**：一个线程内同时监控网络和键盘，实现真正的双向对话，`select()` 确保不阻塞。

---

### 4.3.7 数据发送与接收

```c
// 发送
int n = send(sock_conn, msg, strlen(msg), 0);
// 参数：套接字、数据、长度、 flags（通常为0）
// 返回值：实际发送的字节数

// 接收
int n = recv(sock_conn, buf, MAX_BUFFER_SIZE - 1, 0);
if (n > 0) {
    buf[n] = '\0';    // 手动加字符串结束符
    printf("%s", buf);
}
```

---

### 4.3.8 关闭连接

```c
// 检测 quit 消息
if (strcmp(recv_buf, "quit\r\n") == 0 || strcmp(recv_buf, "quit") == 0) {
    send(sock_conn, "Goodbye!\r\n", ...);   // 发送告别消息
    connected = 0;                            // 退出循环
}
closesocket(sock_conn);                       // 关闭连接套接字
// 服务器主线程继续 while(1)，回到 accept 等待下一个客户端
```

---

## 4.4 实验结果及分析

### 4.4.1 服务器启动

```
======================================
     TCP Server - Network Experiment
======================================
Listening port: 6000

[Server] Waiting for connection...
```

**分析**：服务器成功初始化 Winsock，创建 TCP 套接字并绑定端口 6000，开始监听。`INADDR_ANY` 确保接受任意网络接口的连接请求。

---

### 4.4.2 客户端连接成功

```
Choice: 1
[Client] Connecting to 127.0.0.1:6000...
[Client] Connected successfully!
Welcome! You are connected to TCP Server.
```

服务器端显示：
```
[Server] Client connected from: 127.0.0.1:52345
```

**分析**：`connect()` 成功完成 TCP 三次握手，连接建立。服务器通过 `accept()` 获取客户端地址，`inet_ntoa()` 正确转换并显示客户端 IP 和系统分配的临时端口（52345）。

---

### 4.4.3 双向消息传递

**客户端发送，服务器接收**：
```
Choice: 2
You: Hello Server!
[Client] Sent 13 bytes.
[Server] ACK: Hello Server!
```

**服务器主动发送，客户端接收**（在服务器终端直接输入）：
```
You: How are you today?
[Server] Sent: How are you today?
```

客户端收到：
```
[Server] How are you today?
```

**分析**：
- 客户端主线程 `send()` 发送消息，`recv_thread` 独立线程通过 `select()` 非阻塞检测并 `recv()` 接收服务器消息
- 服务器 `handle_client` 线程内，`_kbhit()` 检测键盘输入，`fgets()` 读取，`send()` 发送
- 双方消息互不阻塞，验证了多线程双向通信的正确性

---

### 4.4.4 quit 断开连接

```
Choice: 2
You: quit
[Client] Disconnecting...
```

服务器端：
```
[Server] Client: quit
[Server] Client requested to quit.
[Server] Waiting for next connection...
```

**分析**：`strcmp()` 正确检测 `quit` 消息，触发断开流程。服务器不退出，继续监听，可接受下一个客户端，体现了服务器持续服务的特性。

---

### 4.4.5 实验总结

| 测试项 | 预期结果 | 实际结果 | 结论 |
|--------|---------|---------|------|
| 服务器启动 | 显示监听提示 | 正常显示 | ✅ |
| 客户端连接 | 显示连接成功 | 正常显示 | ✅ |
| 显示客户端 IP | 显示 IP 和端口 | `127.0.0.1:xxxxx` | ✅ |
| 双向消息传递 | 双方都能收发 | 正常 | ✅ |
| quit 断开 | 双方正确关闭 | 正常 | ✅ |
| 服务器继续监听 | 不退出 | 正常 | ✅ |

---

## 4.5 设计中的问题及体会

### 问题一：编译环境配置困难

**问题**：最初尝试在 VS Code 中使用 Code Runner 编译，配置复杂，多次遇到 "Compiler executable not found" 错误。

**解决**：使用 MSVC 编译器，通过批处理脚本调用 `vcvarsall.bat` 设置环境，成功编译。

**体会**：编译环境的配置是 Windows C 语言网络编程的第一步，需要理解编译器、链接库、系统头文件之间的关系。

---

### 问题二：双向通信的单线程瓶颈

**问题**：最初的单线程版本只能"客户端发 → 服务器收 → 服务器发 → 客户端收"，无法同时主动发消息。

**解决**：引入多线程——服务器每接受一个客户端创建独立线程，线程内用 `select()` 检测网络、`_kbhit()` 检测键盘，实现真正双向对话。

**体会**：多线程是网络编程中实现并发的核心技术，合理使用可以大大提升程序的交互能力。

---

### 问题三：端口和字节序转换

**问题**：早期代码未使用 `htons()`，端口绑定失败；未使用 `inet_ntoa()`，IP 显示为乱码。

**解决**：
- `htonl/htons`：主机字节序转网络字节序（大端序）
- `ntohs/ntohl`：网络字节序转主机字节序
- `inet_ntoa`：网络字节序 IP 转为可读字符串

**体会**：不同系统的字节序可能不同，网络协议统一规定了大端序，编程时必须进行转换。

---

### 总体体会

通过本次综合实验，我有以下收获：

1. **理论联系实际**：课本上学的 TCP 三次握手四次挥手、socket 编程接口，在实际代码中一一对应，加深了对协议的理解。

2. **分层思想的理解**：应用层只关心业务逻辑（发送消息、quit 断开），运输层的可靠性保证由 TCP 协议和操作系统处理，网络层 IP 地址由 `inet_ntoa` 转换，分层使得复杂问题简单化。

3. **多线程的实用价值**：多线程让程序可以同时做多件事（一边收消息一边发消息），这是现代网络程序的常见模式。

4. **调试和查错能力**：编译错误、网络连接失败等问题需要仔细阅读错误码（`WSAGetLastError()`），培养了独立解决问题的能力。

5. **安全意识**：目前程序没有身份验证和数据加密，实际应用中需要考虑用户名密码认证和 SSL/TLS 加密，这是后续可以改进的方向。