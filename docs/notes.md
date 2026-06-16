# 笔记

---

## Winsock2 是什么

**Winsock2** = Windows Sockets API Version 2，Windows 平台的网络编程接口。

- **头文件**：`#include <winsock2.h>`
- **库文件**：`ws2_32.lib`（MSVC 可用 `#pragma comment(lib, "ws2_32.lib")`）
- **初始化**：`WSAStartup(version, &wsa_data)` — 必须先调用，才能用 socket 函数
- **清理**：`WSACleanup()` — 程序退出前调用，释放资源
- **版本**：`MAKEWORD(2, 2)` 请求 Winsock 2.2，目前最常用
- **为什么需要初始化**：Winsock 是动态链接库，不打开它就没法调用 `socket()` 等函数
- **和 Linux 的区别**：Linux 用 BSD socket（`#include <sys/socket.h>`），API 相似但不兼容

本程序中 `init_winsock()` 函数（server.c:158 / client.c:171）完成初始化，程序结束前调用 `WSACleanup()`。

---

## MinGW 和 MSVC 是什么

- **MinGW**：Windows 上的 GNU 工具链（gcc），开源免费，编译参数 `-lws2_32`
- **MSVC**：微软 Visual C++ 编译器（cl.exe），随 VS 安装，链接 `ws2_32.lib`
- 本项目用 MSVC，编译前需先运行 `vcvarsall.bat x64` 配置环境

---

## IP 地址和端口 55873、6000 的含义

- **服务器端口 6000**：`bind()` 绑定的服务端口，监听客户端连接请求。0-1023 是知名端口（HTTP 80），需管理员权限；6000 在 1024-49151 注册端口范围内，可自由使用
- **客户端端口 55873**：客户端连接时，操作系统自动为客户端分配的**临时端口**，用于这次通信，连接断开后释放
- **为什么连接后显示 55873 而不是 6000**：因为 6000 是服务器端口，连接建立后，服务器用 55873 这个客户端临时端口来发回数据
- **127.0.0.1**：本机回环地址，客户端和服务器都在同一台电脑上

---

## 无连接（UDP）与面向连接（TCP）套接字的区别

| | 无连接套接字（SOCK_DGRAM / UDP） | 面向连接套接字（SOCK_STREAM / TCP） |
|---|---|---|
| **是否需要 connect** | 不需要，直接 `sendto`/`recvfrom` 发收 | 需要先 `connect`，之后 `send`/`recv` |
| **是否有三次握手** | 无，发送时直接发 | 有，建立可靠连接（三次握手） |
| **是否保证送达** | 不保证，可能丢包、乱序 | 保证可靠、有序、无重复 |
| **服务器是否 listen/accept** | 不需要 | 需要 `listen` + `accept` |
| **适合场景** | 实时性要求高、容忍丢包（视频、语音） | 可靠性要求高（文件传输、消息） |
| **本程序使用** | 否 | 是（`SOCK_STREAM`） |

**本程序使用 TCP（面向连接）**：服务器 `socket → bind → listen → accept`，客户端 `socket → connect`，之后双方 `send/recv`。

---

## 为什么用 TCP 而不是 UDP

- **UDP 无连接，直接发数据**：客户端 `sendto()` 发完就走，不确认对方是否收到。本程序是交互式对话，必须确认消息送达
- **UDP 不保证顺序**：先发的包可能后到，对话消息乱序体验很差
- **UDP 丢包不重传**：网络波动时消息丢失，用户体验无法接受
- **TCP 保证可靠传输**：每条消息都有确认，丢包自动重传，保证对话完整

**本程序选 TCP 的原因**：交互式聊天需要消息可靠送达且有序，TCP 天然满足这些需求。

**如果做视频通话/游戏**：用 UDP，因为可以容忍少量丢包，追求低延迟，丢了就跳帧或下一帧补上。

---

## .exe 文件是怎么生成的（编译过程）

`.exe` 不是写完 `.c` 就有的，需要执行**编译命令**才会生成：

```
写代码(server.c) → 手动执行编译命令 → 生成.exe
```

### 编译过程（4 步）

| 步骤 | 工具 | 做什么 |
|------|------|------|
| 1. 预处理 | 预处理器 | 展开 `#include`、`#define`，处理宏 |
| 2. 编译 | 编译器（cl.exe） | 将 C 代码翻译成汇编，生成 `.obj` 目标文件 |
| 3. 汇编 | 汇编器 | 将汇编转为机器码（`.obj`） |
| 4. 链接 | 链接器 | 把 `.obj` 和 `ws2_32.lib` 合并，生成 `.exe` |

### 本项目编译命令

```
cl /EHsc /Fe:bin\server.exe src\server.c ws2_32.lib
```
- `/EHsc`：开启异常处理和 C++ 风格栈展开
- `/Fe:bin\server.exe`：指定输出文件路径
- `ws2_32.lib`：链接 Windows 网络库（Winsock）

**执行命令后**，才会生成 `bin/server.exe`。不执行命令，`.c` 文件只是文本，不会变成程序。

### 编译产物

```
bin/server.exe   ← 最终可执行程序
bin/server.obj   ← 中间产物（机器码，未链接）
```

---

## 本项目实际编译过程

1. 写完 `server.c` 后，需要手动执行编译命令才能生成 `.exe`
2. 本项目使用 **MSVC**（cl.exe），编译器路径 `D:\VS2022\VC\Tools\MSVC\...\cl.exe`
3. 编译前需先执行 `vcvarsall.bat x64` 配置 MSVC 环境
4. 执行编译命令：
   ```
   call "D:\VS2022\VC\Auxiliary\Build\vcvarsall.bat" x64
   cl /EHsc /Fe:bin\server.exe src\server.c ws2_32.lib
   ```
5. 命令执行后，才会在 `bin/` 目录下生成 `server.exe` 和 `server.obj`

---

## 加深网络协议概念的理解

### 协议是什么

**协议 = 约定好的通信规则**。双方按照同一套规则通信，才能互相听懂。

本程序涉及的核心协议规则：
- **TCP 协议**：三次握手建立连接 → 可靠传输 → 四次挥手断开
- **socket 接口**：程序员不直接操作 TCP 头，而是调用 socket API，按协议规定的方式调用

### 协议的分层实现

| 课程层次 | 本程序对应 |
|---------|---------|
| 应用层 | `send()`/`recv()`，业务逻辑（发消息、quit） |
| 运输层 | TCP 协议（三次握手、ACK确认、流控、拥塞控制） |
| 网络层 | IP 地址（127.0.0.1）、`inet_ntoa()` 转换 |
| 数据链路层 | 本程序不涉及（由操作系统/网卡处理） |
| 物理层 | 本程序不涉及（网线、网卡硬件） |

### 本实验对协议设计的理解

1. **为什么分层？** — 每一层只管自己的事，应用层不用关心三次握手怎么实现，降低复杂度
2. **为什么 TCP 要三次握手？** — 确认双方都能收发，避免一方还没准备好就发数据
3. **socket 的作用** — 封装了运输层协议细节，提供统一接口，让程序员不用写 TCP 头部的每个字节
4. **协议与代码的对应** — `socket()` 创建端点，`bind()` 绑定地址端口，`listen()` 宣告监听，`accept()` 等待连接，每个函数都对应协议规定的一个步骤

---

## ACK 回复是什么

- **ACK** = Acknowledge，"确认" 的意思
- 服务器收到客户端消息后，回一条 `[Server] ACK: xxx`，意思是"我收到了"
- 名字来源：TCP 协议中，接收方每收到一个数据包都会返回一个 **ACK 包**（确认包），告诉发送方"这条消息我收到了"
- 本程序里的 ACK 是**应用层的简单回显**，是代码写的，不是 TCP 协议自动发的

**TCP 协议层的 ACK**：三次握手、滑动窗口等都用 ACK 标志位（TCP 头第 4 位），由操作系统自动处理

---

## TCP 三次握手、四次挥手在代码中的对应

### 三次握手（建立连接）

课本上的三次握手，在代码中各阶段对应：

```
客户端                                    服务器
  |                                        |
  |  1. 主动 connect()                     |  socket() + bind() + listen() 等待
  |     ── SYN=1, seq=x ──────────────>    |   2. accept() 阻塞等待
  |                                        |
  |  3. connect() 返回成功                 |   4. accept() 返回 sock_conn
  |     <── SYN=1, ACK=1, seq=y, ack=x+1 ──|
  |                                        |
  |     ── ACK=1, seq=x+1, ack=y+1 ────>  |
  |                                        |
  |             连接建立成功                 |
  |           send() / recv()              |
```

| 课文知识点 | 代码对应 | 文件行号 |
|-----------|---------|---------|
| 服务器准备好，打开端口 | `socket()` + `bind()` + `listen()` | server.c:169-197 |
| 服务器被动等待连接 | `accept()` 阻塞等待 | server.c:54 |
| 客户端发送 SYN | `connect()` 内部自动完成 | client.c:205 |
| 服务器回复 SYN+ACK | 操作系统自动处理，程序员感知不到 | 隐含在 `accept()` 返回中 |
| 客户端发送 ACK | `connect()` 返回成功表示握手完成 | client.c:208 |
| 连接建立后通信 | `send()` / `recv()` | server.c:105-134 |

**面试重点**：三次握手在代码层面是 `connect()` 触发，`accept()` 返回后连接已建立。三次握手的具体报文由操作系统 TCP 协议栈自动完成，程序员看不到，但需知道这个过程在调用 `connect()`/`accept()` 时发生。

### 四次挥手（断开连接）

```
客户端                                   服务器
  |                                        |
  |  1. send("quit") / closesocket()       |   2. recv() 返回 quit 消息
  |     ── FIN=1, seq=u ──────────────>    |
  |                                        |
  |  3.                              4. send("Goodbye!")
  |     <── ACK=1, ack=u+1 ──────────────  |
  |                                        |
  |  5.                              6. recv 返回 0
  |     <── FIN=1, seq=w ────────────────  |
  |                                        |
  |  7. send ACK                           |
  |     ── ACK=1, ack=w+1 ──────────────>  |
  |                                        |
  |  8. closesocket()                9. closesocket()
  |     等待 2MSL 后关闭                    |
```

| 课文知识点 | 代码对应 | 文件行号 |
|-----------|---------|---------|
| 第一次挥手（客户端发 FIN） | `send("quit")` 或 `closesocket()` | client.c:85-91 |
| 第二次挥手（服务器回 ACK） | `recv()` 收到消息后，TCP 协议层自动发 ACK | server.c:105 (自动) |
| 第三次挥手（服务器发 FIN） | `closesocket(sock_conn)` 触发 | server.c:147 |
| 第四次挥手（客户端回 ACK） | TCP 协议层自动处理，`closesocket()` 释放资源 | client.c:90 (自动) |
| 2MSL 等待 | 操作系统自动完成，程序员无需操作 | 隐含在 `closesocket()` 后 |
| **本程序 quit 检测** | `strcmp(msg, "quit\r\n")` 应用层检测，触发断开流程 | server.c:111-116 |

### 代码实现总结

| 课本概念 | 代码函数 | 谁处理 |
|---------|---------|--------|
| 三次握手 | `connect()` + `accept()` | 操作系统 TCP 协议栈自动完成 |
| 四次挥手 | `closesocket()` + `recv()==0` | 操作系统 TCP 协议栈自动完成 |
| 可靠传输 | `send()` + `recv()` | 操作系统保证顺序/重传/确认 |
| 应用层断开逻辑 | `strcmp("quit")` + `connected=0` | 程序员自己写代码实现 |

**核心理解**：TCP 协议的底层细节（SYN/FIN/ACK 报文、重传、滑动窗口、拥塞控制）都由操作系统 TCP 协议栈自动完成。socket 编程接口把这些复杂操作封装成简单函数，程序员只需要在正确的时机调用正确的函数即可。

---

## 双向消息传递的实现原理

### 1. 问题：为什么需要多线程？

单线程方案存在根本性缺陷：`recv()` 是**阻塞调用**——当程序调用 `recv()` 时，程序会暂停等待数据到达。在此期间无法执行其他操作（比如检测键盘输入、发送消息）。

以一个简单的单线程服务器为例：

```c
while (1) {
    // 阻塞等待客户端发消息，在此期间：
    // - 服务器无法主动发消息给客户端
    // - 只能等客户端先发，服务器才能回复
    n = recv(sock_conn, buf, sizeof(buf), 0);

    // 服务器回消息
    send(sock_conn, "ACK", 4, 0);
}
```

这种模式下，**双方无法同时主动发消息**：客户端发一条，服务器收一条，服务器才能回一条，客户端才能收到。无法实现真正的"双向同时对话"。

---

### 2. 解决方案：多线程

本程序引入**多线程**，将"接收消息"和"发送消息"分别放在不同的执行流中，各自有独立的指令指针，可以同时运行。

#### 服务器端：每客户端一个线程

```c
// src/server.c 第 50-78 行：主线程
while (1) {
    sock_conn = accept(...);              // 接受连接
    send_greeting(sock_conn);             // 发欢迎消息

    // 创建独立线程处理这个客户端，主线程继续回去 accept 下一个
    HANDLE thread = _beginthreadex(
        NULL, 0, handle_client,
        (void *)sock_conn,
        0, NULL);
    CloseHandle(thread);
}
```

`handle_client()` 线程函数负责与这个客户端的双向通信，主线程回到 `accept()` 等待下一个客户端，不被阻塞。

#### 客户端：发送线程 + 接收线程分离

```c
// src/client.c 第 60-66 行
if (connect_server(g_sock, DEFAULT_IP, DEFAULT_PORT) == 0) {
    // 连接成功后创建接收线程
    HANDLE thread = _beginthreadex(
        NULL, 0, recv_thread, NULL, 0, NULL);
    CloseHandle(thread);
}
```

- **主线程**：负责菜单交互、用户输入、发送消息（case 2）
- **recv_thread 线程**：独立接收服务器消息，不阻塞主线程

两者并发运行，互不干扰，实现了客户端的双向通信。

---

### 3. 线程内部如何同时处理收和发

线程内同时监控两件事：**网络上有没有数据到达** 和 **键盘有没有输入**。

#### select()：非阻塞检测网络数据

`recv()` 是阻塞的，直接调用会卡住。用 `select()` 可以**非阻塞检测** socket 是否有数据可读：

```c
// src/server.c 第 93-126 行
while (connected) {
    fd_set readfds;
    struct timeval tv;
    FD_ZERO(&readfds);
    FD_SET(sock_conn, &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 100000;  // 0.1 秒超时

    int ret = select(0, &readfds, NULL, NULL, &tv);

    if (ret > 0 && FD_ISSET(sock_conn, &readfds)) {
        // 有数据到达，可以安全地 recv，不会阻塞
        int n = recv(sock_conn, recv_buf, MAX_BUFFER_SIZE - 1, 0);
        if (n > 0) {
            recv_buf[n] = '\0';
            printf("[Server] Client: %s", recv_buf);
            // ... 处理消息 ...
        }
    }
}
```

- `select()` 返回正数表示有 socket 可读，返回 0 表示超时（无数据）
- 设置 `tv.tv_usec = 100000`（0.1 秒）作为超时，返回后线程继续循环
- 如果 `select()` 判定有数据可读，此时 `recv()` 必然有数据，**不会阻塞**
- 客户端 `recv_thread()`（`src/client.c` 第 117-148 行）使用完全相同的模式

#### _kbhit()：非阻塞检测键盘输入

服务器端还需要能主动发消息给客户端。在线程循环中加入 `_kbhit()` 检测键盘：

```c
// src/server.c 第 128-144 行
if (_kbhit()) {
    memset(input_buf, 0, sizeof(input_buf));
    fgets(input_buf, sizeof(input_buf), stdin);

    if (strlen(input_buf) > 0) {
        send(sock_conn, input_buf, (int)strlen(input_buf), 0);
        printf("[Server] Sent: %s", input_buf);

        if (strcmp(input_buf, "quit\r\n") == 0 ||
            strcmp(input_buf, "quit") == 0) {
            connected = 0;
        }
    }
}
```

- `_kbhit()` 是 Windows 函数，无键盘输入时立即返回 0，不阻塞
- 有键盘输入时读取内容，通过 `send()` 主动发给客户端

### 4. 时序图：单线程 vs 多线程

**单线程（阻塞版）**：

```
客户端    ──> 发消息    ──────────────────────────────────>
服务器              <-- recv 阻塞等待 --
                     -- 处理完成 -->   <-- recv 再次阻塞等待 --
                                   服务器发消息 -->  ← 只有等 recv 返回后才能 send
客户端                                          <-- 收到消息 --
```

**多线程（本程序）**：

```
服务器线程（独立执行流）
    recv_thread：       select 等待 ── 有数据 ── recv ── 处理
    send（通过 _kbhit）:    检测键盘 ── 有输入 ── send

同时进行，互不阻塞：
    ──────────────────────────────────────────────────────>
                                                    服务器线程 ──────>
                                                    服务器线程 ──────>
```

---

### 5. 本程序与课程知识的对应

| 课程知识点 | 程序实现 | 代码位置 |
|-----------|---------|---------|
| 线程概念 | 每客户端一个线程独立处理 | server.c:67-70 |
| 并发 | 多线程并发执行 | server.c:86-150 |
| I/O 多路复用 | `select()` 非阻塞检测 | server.c:102 / client.c:128 |
| 阻塞 vs 非阻塞 I/O | `select()` + 超时实现非阻塞 | server.c:93-126 |
| 客户-服务器模型 | 主线程 accept，线程处理客户端 | server.c:50-78 |
| 全双工通信 | 双向同时收发 | 线程 + select + _kbhit |

---

### 6. 总结

**为什么需要多线程 + select + _kbhit**：

1. **`_beginthreadex()`**：为每个客户端创建独立执行流，使服务器能同时服务多个客户端
2. **`select()`**：在接收线程中非阻塞检测 socket 状态，有数据再 recv，避免线程卡死
3. **`_kbhit()`**：在接收线程中非阻塞检测键盘，允许服务器随时主动发消息给客户端

三者结合，实现了真正的**双向同时对话**——双方都能在任何时候主动发消息，不受对方速度限制。这是网络聊天程序的核心需求，也是本实验区别于基础版本的核心亮点。

---

## 程序线程结构详解

本程序一共涉及 **3 个线程**，分两个地方：

### 服务器端：2 个线程

```
主线程 ──────────────────────────── 一直循环，监听端口
│ accept() 阻塞等客户端连接
│ 有客户端来了 → 创建 handle_client 线程
│ 立刻回去 accept()，不等 handle_client
└─ 永远不会结束，除非手动关程序
```

```
handle_client 线程（每客户端一个）
│
├─ select() + recv()  ── 接收该客户端发来的消息
│                         收到 quit → 发 Goodbye → 线程结束
│
└─ _kbhit() + send()  ── 检测服务器键盘输入
                           发给该客户端
                           输入 quit → 线程结束
```

**举例**：来了 2 个客户端，就有 2 个 `handle_client` 线程同时跑，主线程继续等第 3 个。

### 客户端端：2 个线程

```
主线程 ──────────────────────────── 显示菜单循环
│
├─ 选项 1: connect_server() 连接服务器
│          连接成功后 → 创建 recv_thread → 继续循环
│
├─ 选项 2: 用户输入 → send() 发消息给服务器
│          quit → closesocket() 断开
│
├─ 选项 3: 发 quit → 断开
│
└─ 选项 4: cleanup() → exit(0)
          程序结束
```

```
recv_thread 线程（1 个）
│
└─ select() + recv()  ── 持续监听服务器发来的消息
                           有数据 → 打印 [Server] xxx
                           断开 → 线程结束
```

**主线程和 recv_thread 同时运行**：主线程让你输入发消息，recv_thread 同时接收服务器的消息，两者不互相等待。

### 一张图总结

```
服务器                          客户端
┌─────────────┐                ┌─────────────┐
│  主线程      │                │  主线程      │
│  accept()   │                │  菜单+send() │
│  循环        │                │  循环        │
└──────┬──────┘                └──────┬──────┘
       │ 创建线程                      │ 创建线程
       ▼                              ▼
┌─────────────┐                ┌─────────────┐
│ handle_client│               │ recv_thread │
│ select+recv │                │ select+recv │
│ _kbhit+send │                │             │
└─────────────┘                └─────────────┘
```

**核心区别**：
- 服务器：主线程**只做 accept**，通信全在子线程
- 客户端：主线程**负责发送**，接收在子线程

---

## 线程相关函数

### `_beginthreadex()` / `_endthreadex()`

Windows 线程创建和退出函数，属于 **C 运行时库（CRT）**，需 `#include <process.h>`。

```c
// 创建线程
#include <process.h>

uintptr_t _beginthreadex(
    void *security,      // 安全属性，填 NULL
    unsigned stack_size, // 堆栈大小，0 表示默认
    unsigned (__stdcall *start_addr)(void*), // 线程函数
    void *arglist,       // 传给线程函数的参数
    unsigned initflag,   // 创建标志，0 表示立即运行
    unsigned *thrdaddr   // 线程 ID，填 NULL
);

// 线程函数返回时自动调用
void _endthreadex(unsigned retval);
```

**为什么用 `_beginthreadex` 而不是 `CreateThread`**：CRT 可能在内部维护线程局部数据，`_beginthreadex` 会初始化这些数据，避免与 CRT 冲突。直接用 `CreateThread` 可能导致内存泄漏。应始终使用 `_beginthreadex`。

本程序用法：
```c
// server.c:67 / client.c:61
HANDLE thread = _beginthreadex(NULL, 0, handle_client, (void *)sock_conn, 0, NULL);
CloseHandle(thread);  // 关闭句柄，线程继续独立运行
```

---

### `select()`

I/O 多路复用函数，属于 **Winsock2（ws2_32）**，需 `#include <winsock2.h>`。

```c
int select(
    int nfds,                      // 最大 socket 句柄值 +1（填 0 即可）
    fd_set *readfds,               // 监控可读的 socket 集合（可为 NULL）
    fd_set *writefds,              // 监控可写的 socket 集合（可为 NULL）
    fd_set *exceptfds,             // 监控异常的 socket（可为 NULL）
    const struct timeval *timeout  // 超时时间（可为 NULL 表示永久阻塞）
);

// struct timeval 定义
struct timeval {
    long tv_sec;   // 秒
    long tv_usec;  // 微秒
};
```

- 返回值：>0 表示有 socket 触发事件；0 表示超时；<0 表示出错
- 本程序用 `select()` 实现非阻塞检测：设 0.1 秒超时，每次等 0.1 秒看 socket 有没有数据，有则 `recv()` 安全不会卡住，无则继续循环

```c
fd_set readfds;
struct timeval tv = {0, 100000};  // 0 秒 + 10万微秒 = 0.1 秒
FD_ZERO(&readfds);
FD_SET(sock_conn, &readfds);
int ret = select(0, &readfds, NULL, NULL, &tv);
if (ret > 0 && FD_ISSET(sock_conn, &readfds)) {
    // socket 有数据，可以安全 recv()
}
```

---

### `FD_SET()` / `FD_ZERO()` / `FD_ISSET()`

操作 `fd_set`（socket 集合）的宏，属于 **Winsock2**，直接包含 `<winsock2.h>` 即可用。

```c
void FD_ZERO(fd_set *set);                // 清空集合
void FD_SET(SOCKET s, fd_set *set);       // 把 socket 加入集合
int  FD_ISSET(SOCKET s, fd_set *set);     // 检测 socket 是否在集合中
```

- `FD_ZERO` 初始化空集合
- `FD_SET` 将要监控的 socket 加入集合
- `FD_ISSET` 判断 `select()` 返回后，某个 socket 是否有事件

---

### `recv()`

TCP 接收函数，属于 **Winsock2**。

```c
int recv(
    SOCKET s,      // 套接字
    char *buf,     // 接收缓冲区
    int len,       // buf 最大长度
    int flags      // 标志，通常为 0
);
// 返回：>0 实际收到的字节数；0 对端关闭连接；SOCKET_ERROR(-1) 出错
```

- **阻塞**：调用后程序暂停等待数据，有数据才返回
- 本程序中配合 `select()` 先行检测：有数据再调用 `recv()`，避免卡住
- `recv()` 不保证收到完整的消息边界，TCP 是字节流协议

```c
// server.c:105
int n = recv(sock_conn, recv_buf, MAX_BUFFER_SIZE - 1, 0);
if (n > 0) {
    recv_buf[n] = '\0';  // 手动加字符串结束符
    printf("[Server] Client: %s", recv_buf);
}
```

---

### `send()`

TCP 发送函数，属于 **Winsock2**。

```c
int send(
    SOCKET s,          // 套接字
    const char *buf,   // 要发送的数据
    int len,           // 数据长度
    int flags          // 标志，通常为 0
);
// 返回：>0 实际发送的字节数；SOCKET_ERROR(-1) 出错
```

- 成功返回实际发送的字节数（可能小于 len），不保证一次发完
- 本程序用于发消息给对端，以及 quit 后发 Goodbye 消息

```c
// server.c:114
const char *bye = "Goodbye!\r\n";
send(sock_conn, bye, (int)strlen(bye), 0);

// client.c:78
send(g_sock, msg, (int)strlen(msg), 0);
```

---

### `_kbhit()`

检测键盘是否有按键等待读取，属于 **Windows C 运行时库**，需 `#include <conio.h>`。

```c
#include <conio.h>

int _kbhit(void);
// 返回：有按键返回非零值；无按键返回 0
```

- **非阻塞**：立即返回，不等待用户输入
- 本程序用于在消息循环中检测服务器用户是否想主动发消息

```c
// server.c:129
if (_kbhit()) {
    fgets(input_buf, sizeof(input_buf), stdin);
    send(sock_conn, input_buf, (int)strlen(input_buf), 0);
}
```

---

### `fgets()`

从标准输入读取一行字符串，属于 **C 标准库**，需 `#include <stdio.h>`。

```c
#include <stdio.h>

char *fgets(char *s, int size, FILE *stream);
// s:      存储读取内容
// size:   最大读取字符数（含 \0）
// stream: 文件流，stdin 表示键盘输入
// 返回：成功返回 s，失败或 EOF 返回 NULL
```

- 读取直到换行符 `\n` 或达到 `size-1` 个字符
- 会保留换行符 `\n`，需要 `strcmp()` 时注意加上
- 本程序配合 `_kbhit()` 使用：检测到按键后读取内容

```c
// server.c:131
fgets(input_buf, sizeof(input_buf), stdin);
// 读取内容包含换行符，如 "Hello\n"
// strcmp(input_buf, "Hello\r\n") 才匹配（回车+换行）
```

---

### `closesocket()`

关闭套接字，属于 **Winsock2**。

```c
int closesocket(SOCKET s);
// 返回：0 成功；SOCKET_ERROR(-1) 出错
```

- 关闭后 socket 不再可用，所有待发数据尽可能发送后连接关闭
- 服务器端：每个客户端连接线程结束时调用，关闭与该客户端的 `sock_conn`
- 客户端主动断开时也调用，会触发服务器端 `recv()` 返回 0（对端关闭）

```c
// server.c:147（线程结束时）
closesocket(sock_conn);

// client.c:84（quit 断开）
closesocket(g_sock);
g_sock = INVALID_SOCKET;
```

---

## 函数速查表

| 函数 | 所属库 | 作用 |
|------|--------|------|
| `_beginthreadex()` | `<process.h>` CRT | 创建线程 |
| `_endthreadex()` | `<process.h>` CRT | 线程主动退出 |
| `select()` | `<winsock2.h>` | 非阻塞检测 socket 状态 |
| `FD_SET/FD_ZERO/FD_ISSET` | `<winsock2.h>` 宏 | 操作 socket 集合 |
| `recv()` | `<winsock2.h>` | 接收网络数据（阻塞，需先 select） |
| `send()` | `<winsock2.h>` | 发送网络数据 |
| `_kbhit()` | `<conio.h>` CRT | 非阻塞检测键盘按键 |
| `fgets()` | `<stdio.h>` stdlib | 读取一行输入（保留换行） |
| `closesocket()` | `<winsock2.h>` | 关闭套接字 |

---

## 双向消息传递的实现原理

### 1. 问题：为什么需要多线程？

单线程方案存在根本性缺陷：`recv()` 是**阻塞调用**——当程序调用 `recv()` 时，程序会暂停等待数据到达。在此期间无法执行其他操作（比如检测键盘输入、发送消息）。

以一个简单的单线程服务器为例：

```c
while (1) {
    // 阻塞等待客户端发消息，在此期间：
    // - 服务器无法主动发消息给客户端
    // - 只能等客户端先发，服务器才能回复
    n = recv(sock_conn, buf, sizeof(buf), 0);

    // 服务器回消息
    send(sock_conn, "ACK", 4, 0);
}
```

这种模式下，**双方无法同时主动发消息**：客户端发一条，服务器收一条，服务器才能回一条，客户端才能收到。无法实现真正的"双向同时对话"。

---

### 2. 解决方案：多线程

本程序引入**多线程**，将"接收消息"和"发送消息"分别放在不同的执行流中，各自有独立的指令指针，可以同时运行。

#### 服务器端：每客户端一个线程

```c
// src/server.c 第 50-78 行：主线程
while (1) {
    sock_conn = accept(...);              // 接受连接
    send_greeting(sock_conn);             // 发欢迎消息

    // 创建独立线程处理这个客户端，主线程继续回去 accept 下一个
    HANDLE thread = _beginthreadex(
        NULL, 0, handle_client,
        (void *)sock_conn,
        0, NULL);
    CloseHandle(thread);
}
```

`handle_client()` 线程函数负责与这个客户端的双向通信，主线程回到 `accept()` 等待下一个客户端，不被阻塞。

#### 客户端：发送线程 + 接收线程分离

```c
// src/client.c 第 60-66 行
if (connect_server(g_sock, DEFAULT_IP, DEFAULT_PORT) == 0) {
    // 连接成功后创建接收线程
    HANDLE thread = _beginthreadex(
        NULL, 0, recv_thread, NULL, 0, NULL);
    CloseHandle(thread);
}
```

- **主线程**：负责菜单交互、用户输入、发送消息（case 2）
- **recv_thread 线程**：独立接收服务器消息，不阻塞主线程

两者并发运行，互不干扰，实现了客户端的双向通信。

---

### 3. 线程内部如何同时处理收和发

线程内同时监控两件事：**网络上有没有数据到达** 和 **键盘有没有输入**。

#### select()：非阻塞检测网络数据

`recv()` 是阻塞的，直接调用会卡住。用 `select()` 可以**非阻塞检测** socket 是否有数据可读：

```c
// src/server.c 第 93-126 行
while (connected) {
    fd_set readfds;
    struct timeval tv;
    FD_ZERO(&readfds);
    FD_SET(sock_conn, &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 100000;  // 0.1 秒超时

    int ret = select(0, &readfds, NULL, NULL, &tv);

    if (ret > 0 && FD_ISSET(sock_conn, &readfds)) {
        // 有数据到达，可以安全地 recv，不会阻塞
        int n = recv(sock_conn, recv_buf, MAX_BUFFER_SIZE - 1, 0);
        if (n > 0) {
            recv_buf[n] = '\0';
            printf("[Server] Client: %s", recv_buf);
            // ... 处理消息 ...
        }
    }
}
```

- `select()` 返回正数表示有 socket 可读，返回 0 表示超时（无数据）
- 设置 `tv.tv_usec = 100000`（0.1 秒）作为超时，返回后线程继续循环
- 如果 `select()` 判定有数据可读，此时 `recv()` 必然有数据，**不会阻塞**
- 客户端 `recv_thread()`（`src/client.c` 第 117-148 行）使用完全相同的模式

#### _kbhit()：非阻塞检测键盘输入

服务器端还需要能主动发消息给客户端。在线程循环中加入 `_kbhit()` 检测键盘：

```c
// src/server.c 第 128-144 行
if (_kbhit()) {
    memset(input_buf, 0, sizeof(input_buf));
    fgets(input_buf, sizeof(input_buf), stdin);

    if (strlen(input_buf) > 0) {
        send(sock_conn, input_buf, (int)strlen(input_buf), 0);
        printf("[Server] Sent: %s", input_buf);

        if (strcmp(input_buf, "quit\r\n") == 0 ||
            strcmp(input_buf, "quit") == 0) {
            connected = 0;
        }
    }
}
```

- `_kbhit()` 是 Windows 函数，无键盘输入时立即返回 0，不阻塞
- 有键盘输入时读取内容，通过 `send()` 主动发给客户端

### 4. 时序图：单线程 vs 多线程

**单线程（阻塞版）**：

```
客户端    ──> 发消息    ──────────────────────────────────>
服务器              <-- recv 阻塞等待 --
                     -- 处理完成 -->   <-- recv 再次阻塞等待 --
                                   服务器发消息 -->  ← 只有等 recv 返回后才能 send
客户端                                          <-- 收到消息 --
```

**多线程（本程序）**：

```
服务器线程（独立执行流）
    recv_thread：       select 等待 ── 有数据 ── recv ── 处理
    send（通过 _kbhit）:    检测键盘 ── 有输入 ── send

同时进行，互不阻塞：
    ──────────────────────────────────────────────────────>
                                                    服务器线程 ──────>
                                                    服务器线程 ──────>
```

---

### 5. 本程序与课程知识的对应

| 课程知识点 | 程序实现 | 代码位置 |
|-----------|---------|---------|
| 线程概念 | 每客户端一个线程独立处理 | server.c:67-70 |
| 并发 | 多线程并发执行 | server.c:86-150 |
| I/O 多路复用 | `select()` 非阻塞检测 | server.c:102 / client.c:128 |
| 阻塞 vs 非阻塞 I/O | `select()` + 超时实现非阻塞 | server.c:93-126 |
| 客户-服务器模型 | 主线程 accept，线程处理客户端 | server.c:50-78 |
| 全双工通信 | 双向同时收发 | 线程 + select + _kbhit |

---

### 6. 总结

**为什么需要多线程 + select + _kbhit**：

1. **`_beginthreadex()`**：为每个客户端创建独立执行流，使服务器能同时服务多个客户端
2. **`select()`**：在接收线程中非阻塞检测 socket 状态，有数据再 recv，避免线程卡死
3. **`_kbhit()`**：在接收线程中非阻塞检测键盘，允许服务器随时主动发消息给客户端

三者结合，实现了真正的**双向同时对话**——双方都能在任何时候主动发消息，不受对方速度限制。这是网络聊天程序的核心需求，也是本实验区别于基础版本的核心亮点。