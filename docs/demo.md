# Demo 演示步骤

适用于《计算机网络》综合实验现场演示，时长约 5-10 分钟。

---

## 核心亮点（必讲）

本程序相比基础版本有三个重要提升，演示时请重点强调：

### 亮点一：TCP 面向连接协议

```c
// src/server.c 第 171 行
SOCKET s = socket(AF_INET, SOCK_STREAM, 0);   // TCP 流式套接字
// 对比: SOCK_DGRAM = UDP 数据报套接字
```

- `SOCK_STREAM` 表示 TCP，提供可靠、有序、无重复的字节流传输
- 相比 UDP，TCP 需要先建立连接（`connect`/`accept`），适合交互式对话
- 展示给老师看这一行代码即可说明协议选择

### 亮点二：多线程实现真正双向对话

这是本程序的核心创新点。传统单线程方案只能一方发一方收，本程序**双方都能同时主动发消息**：

**服务器端**（每客户端一个线程，`src/server.c`）：

```c
// 第 67-70 行：主线程接受连接后，创建子线程处理
HANDLE thread = (HANDLE)_beginthreadex(
    NULL, 0, handle_client,
    (void *)sock_conn, 0, NULL);

// 第 86-150 行：子线程 handle_client() 同时收发
// 第 102-126 行：select() 非阻塞检测网络数据，recv() 接收客户端消息
int ret = select(0, &readfds, NULL, NULL, &tv);
if (ret > 0 && FD_ISSET(sock_conn, &readfds)) {
    int n = recv(sock_conn, recv_buf, MAX_BUFFER_SIZE - 1, 0);

// 第 129-144 行：_kbhit() 非阻塞检测键盘，send() 主动发消息给客户端
if (_kbhit()) {
    fgets(input_buf, sizeof(input_buf), stdin);
    send(sock_conn, input_buf, (int)strlen(input_buf), 0);
```

**客户端**（`src/client.c`）：

```c
// 第 66-69 行：主线程连接成功后，创建 recv_thread 接收线程
HANDLE thread = (HANDLE)_beginthreadex(
    NULL, 0, recv_thread, NULL, 0, NULL);

// 第 117-151 行：recv_thread 线程独立接收，不阻塞主线程
// 第 133-146 行：select() 非阻塞检测 + recv() 接收
int ret = select(0, &readfds, NULL, NULL, &tv);
if (ret > 0 && FD_ISSET(g_sock, &readfds)) {
    int n = recv(g_sock, recv_buf, MAX_BUFFER_SIZE - 1, 0);
    printf("[Server] %s", recv_buf);

// 第 78-83 行：主线程独立发送，用户在菜单选 2 后直接发
send(g_sock, msg, (int)strlen(msg), 0);
```

### 亮点三：quit 优雅断开，服务器持续服务

```c
// src/server.c 第 111-116 行：检测 quit，退出线程，服务器不退出
if (strcmp(recv_buf, "quit\r\n") == 0 || strcmp(recv_buf, "quit") == 0) {
    send(sock_conn, "Goodbye!\r\n", ...);
    connected = 0;   // 退出子线程循环
}
closesocket(sock_conn);  // 关闭连接套接字
// 主线程 continue，重新 accept 下一个客户端（第 50 行 while 循环）

// src/client.c 第 85-91 行：客户端 quit 断开
if (strcmp(msg, "quit\r\n") == 0 || strcmp(msg, "quit") == 0) {
    printf("[Client] Disconnecting...\n");
    g_connected = 0;
    closesocket(g_sock);
    g_sock = INVALID_SOCKET;
}
```

---

## 演示准备

### 文件清单

```
D:\MYCODE\NET_comprehensive_experiment\
├── bin\server.exe      ← 服务器可执行文件
├── bin\client.exe      ← 客户端可执行文件
├── src\server.c        ← 服务器源代码（198 行）
├── src\client.c        ← 客户端源代码（220 行）
└── docs\
    ├── usage.md        ← 编译运行说明
    └── requirements.md ← 需求与实现对照
```

### 编译命令

```powershell
cd D:\MYCODE\NET_comprehensive_experiment
powershell -Command "& 'D:\VS2022\VC\Auxiliary\Build\vcvarsall.bat' x64 >`$null 2>&1; cl /EHsc /Fe:bin\server.exe src\server.c ws2_32.lib"
powershell -Command "& 'D:\VS2022\VC\Auxiliary\Build\vcvarsall.bat' x64 >`$null 2>&1; cl /EHsc /Fe:bin\client.exe src\client.c ws2_32.lib"
```

或 VS Code 打开 `.c` 文件按 `Ctrl + Alt + N`。

---

## 演示流程（共 7 步）

### Step 1：展示项目结构

打开文件夹，资源管理器展示：

```
src/       ← 源代码
bin/       ← 编译产物（exe + obj）
docs/      ← 文档
.vscode/   ← VS Code 配置
```

**话术**：各位老师好，我完成的是基于 **TCP 协议**的网络通信程序，包含服务器和客户端两部分，采用 **C 语言 + Winsock2**，核心亮点是**多线程实现双向对话**。

---

### Step 2：启动服务器

`src/server.c` → `Ctrl + Alt + N`

```
======================================
     TCP Server - Network Experiment
======================================
Listening port: 6000

[Server] Waiting for connection...
```

**话术**：服务器在 6000 端口监听，用 `socket()` 创建 TCP 套接字，`bind()` 绑定端口，`listen()` 开始监听。

**代码（`src/server.c`）**：

| 行号 | 代码 | 说明 |
|------|------|------|
| 第 171 行 | `socket(AF_INET, SOCK_STREAM, 0)` | 创建 TCP 套接字 |
| 第 177-189 行 | `bind_port()` | 绑定端口 6000 |
| 第 191-197 行 | `start_listen()` | listen 队列长度 5 |
| 第 54 行 | `accept(sock_srv, ...)` | 接受客户端连接 |

---

### Step 3：客户端连接

PowerShell 运行 `.\bin\client.exe`，选 `1`：

```
Choice: 1
[Client] Connected successfully!
Welcome! You are connected to TCP Server.
```

服务器端：

```
[Server] Client connected from: 127.0.0.1:52345
```

**话术**：客户端主动连接，服务器 `accept()` 接受后立刻显示客户端 IP，并用 `inet_ntoa()` 转换地址格式显示。

**代码（`src/server.c`）**：

| 行号 | 代码 | 说明 |
|------|------|------|
| 第 60-62 行 | `inet_ntoa(addr_client.sin_addr)` | 显示客户端 IP |
| 第 152-156 行 | `send_greeting()` 函数 | 发送欢迎消息 |

**代码（`src/client.c`）**：

| 行号 | 代码 | 说明 |
|------|------|------|
| 第 191 行 | `socket(AF_INET, SOCK_STREAM, 0)` | 客户端也用 TCP |
| 第 197-210 行 | `connect_server()` 函数 | connect 到服务器 |

---

### Step 4：客户端发消息

```
Choice: 2
You: Hello Server!
```

服务器端：

```
[Server] Client: Hello Server!
[Server] ACK: Hello Server!
```

**话术**：客户端发、服务器收，`recv()` 接收后回 ACK。`select()` 实现非阻塞检测，无需等待。

**代码（`src/server.c`）**：

| 行号 | 代码 | 说明 |
|------|------|------|
| 第 102 行 | `select(0, &readfds, NULL, NULL, &tv)` | 非阻塞检测 socket 可读 |
| 第 105 行 | `recv(sock_conn, recv_buf, ...)` | 接收客户端消息 |
| 第 118-120 行 | `snprintf()` + `send()` | 发送 ACK 回复 |

---

### Step 5：服务器主动发消息（核心亮点演示）

在服务器终端直接输入：

```
You: How are you today?
[Server] Sent: How are you today!
```

客户端立即收到：

```
[Server] How are you today?
```

**话术**：这就是**多线程**的价值——服务器可以随时主动发消息给客户端，互不阻塞。如果没有多线程，服务器必须等客户端先发，自己才能发。

**代码（`src/server.c`，重点展示）**：

| 行号 | 代码 | 说明 |
|------|------|------|
| 第 67-70 行 | `_beginthreadex(handle_client, sock_conn)` | **每客户端创建独立线程** |
| 第 129 行 | `_kbhit()` | **非阻塞检测键盘输入** |
| 第 131 行 | `fgets(input_buf, stdin)` | 读取服务器端输入 |
| 第 134 行 | `send(sock_conn, input_buf, ...)` | **服务器主动发消息** |

**代码（`src/client.c`）**：

| 行号 | 代码 | 说明 |
|------|------|------|
| 第 66-69 行 | `_beginthreadex(recv_thread, NULL)` | 创建独立接收线程 |
| 第 133-146 行 | `select()` + `recv()` | 线程内非阻塞接收 |

---

### Step 6：quit 断开连接

客户端选 `2` → `quit`：

```
[Client] Disconnecting...
```

服务器端：

```
[Server] Client requested to quit.
[Server] Waiting for next connection...
```

**话术**：`strcmp()` 检测到 `quit` 后，双方关闭 socket，但服务器主进程不退出，**继续监听**，可接待下一个客户端。

**代码（`src/server.c`）**：

| 行号 | 代码 | 说明 |
|------|------|------|
| 第 111-116 行 | `strcmp("quit")` + `connected=0` | 检测 quit，退出线程 |
| 第 50 行 | `while (1) { accept(...) }` | 主线程继续监听新连接 |

**代码（`src/client.c`）**：

| 行号 | 代码 | 说明 |
|------|------|------|
| 第 85-91 行 | `closesocket()` + 退出 | 客户端正确关闭 |

---

### Step 7：代码结构总结

**`src/server.c` 函数索引**：

| 行号 | 函数 | 作用 |
|------|------|------|
| 第 158-167 行 | `init_winsock()` | WSAStartup 初始化 Winsock 2.2 |
| 第 169-175 行 | `create_socket()` | 创建 TCP 套接字 |
| 第 177-189 行 | `bind_port()` | 绑定 INADDR_ANY 和端口 6000 |
| 第 191-197 行 | `start_listen()` | listen，队列长度 5 |
| 第 54 行 | `accept()` | 接受客户端连接 |
| 第 67-70 行 | `_beginthreadex()` | **创建客户端处理线程** |
| 第 152-156 行 | `send_greeting()` | 连接成功后发送欢迎消息 |
| 第 86-150 行 | `handle_client()` | **多线程双向通信核心** |

**`src/client.c` 函数索引**：

| 行号 | 函数 | 作用 |
|------|------|------|
| 第 178-187 行 | `init_winsock()` | WSAStartup |
| 第 189-195 行 | `create_socket()` | 创建 TCP 套接字 |
| 第 153-165 行 | `show_menu()` | 打印 1-4 菜单 |
| 第 197-210 行 | `connect_server()` | connect 到 127.0.0.1:6000 |
| 第 66-69 行 | `_beginthreadex(recv_thread)` | **创建独立接收线程** |
| 第 117-151 行 | `recv_thread()` | **独立非阻塞接收线程** |

**话术**：程序采用模块化设计，函数职责单一。核心是**多线程**——服务器每接受一个客户端就创建一个线程独立处理，该线程内用 `select()` 非阻塞接收网络数据，用 `_kbhit()` 检测键盘输入，实现真正的双向实时对话。

---

## 演讲时间控制

| 环节 | 建议时长 |
|------|---------|
| 核心亮点介绍（Step 1 前） | 1 分钟 |
| 启动服务器 + 客户端连接 | 1 分钟 |
| 双向消息传递（含多线程亮点） | 2 分钟 |
| quit 断开演示 | 1 分钟 |
| 代码结构讲解收尾 | 1-2 分钟 |
| 回答老师问题 | 机动 |

---

## 常见问题备用

**Q：编译报错？**
用 PowerShell 执行上面的编译命令。

**Q：客户端连接失败？**
确认服务器先启动，端口 6000 未被占用，防火墙放行。

**Q：怎么证明是 TCP？**
指着 `src/server.c` 第 171 行（或 `src/client.c` 第 191 行）：
`socket(AF_INET, SOCK_STREAM, 0)` — `SOCK_STREAM` = TCP，`SOCK_DGRAM` = UDP。

**Q：多线程怎么做到的？**
`src/server.c` 第 67-70 行：`_beginthreadex()` 创建线程，主线程 `accept()` 接受连接后交给子线程处理，自己继续监听新连接。