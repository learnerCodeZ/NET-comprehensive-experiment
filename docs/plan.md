# 综合实验程序实现方案

## Context

根据《计算机网络》课程综合实验要求，使用 C 语言实现基于 TCP 的 client/server 网络通信模拟程序，报告后续自行撰写。

课程覆盖：概述、物理层、数据链路层、网络层、运输层、应用层。综合实验重点对应**运输层协议（TCP/UDP）**编程实践。

## 项目目录结构

```
D:/MYCODE/NET_comprehensive_experiment/
├── src/
│   ├── server.c         # TCP 服务器（含多线程双向对话）
│   └── client.c         # TCP 客户端（含菜单 + 多线程接收）
├── bin/                  # 编译输出目录
├── docs/
│   ├── plan.md          # 本计划文件
│   ├── requirements.md  # 原始需求文档
│   ├── usage.md         # 使用方法
│   └── demo.md          # 演示步骤（面向老师展示）
├── .vscode/              # VS Code 配置
├── README.md
└── AGENTS.md
```

## 目标程序

**bin/server.exe** — TCP 服务器
**bin/client.exe** — TCP 客户端

## 需求与实现对照

| 编号 | 原始需求 | 程序实现 |
|------|---------|---------|
| R1 | 服务器和客户端有提示 | `server.c` main() 横幅；`client.c` show_menu() |
| R2 | 服务器监听、显示IP、发问候 | accept + inet_ntoa + send_greeting() |
| R3 | 客户端主动连接、菜单控制 | connect_server() + show_menu() 循环 |
| R4 | 双向消息传递，quit 断开 | 多线程 send/recv，strcmp("quit") 触发断开 |
| R5 | 界面友好，功能分函数 | static 模块化函数设计 |
| R6 | 基于 TCP 协议 | socket(AF_INET, SOCK_STREAM, 0) |

---

## 实现详情

### server.c 函数结构

| 函数 | 职责 |
|------|------|
| `main()` | 初始化 → socket → bind → listen → 循环 accept |
| `handle_client()` (线程) | 多线程处理：非阻塞 recv + _kbhit 检测键盘输入，双向对话 |
| `send_greeting()` | 连接后发送欢迎消息 |
| `init_winsock()` | WSAStartup 初始化 |
| `create_socket()` | socket 创建 |
| `bind_port()` | bind 绑定端口 |
| `start_listen()` | listen 开始监听 |

### client.c 函数结构

| 函数 | 职责 |
|------|------|
| `main()` | 菜单循环，选择 1-4 操作 |
| `recv_thread()` (线程) | 多线程非阻塞接收服务器消息 |
| `show_menu()` | 打印菜单 |
| `get_choice()` | 读取用户输入 |
| `init_winsock()` | WSAStartup |
| `create_socket()` | socket 创建 |
| `connect_server()` | connect 到服务器 |
| `cleanup()` | closesocket + WSACleanup |

### 关键实现细节

- **端口**: 6000，服务器 IP: 127.0.0.1
- **多线程**: `_beginthreadex` / `_endthreadex`，服务器每客户端一个线程，客户端主线程发、recv_thread 收
- **非阻塞**: `select()` 检测 socket 可读 + `_kbhit()` 检测服务器键盘输入
- **编码**: 源文件 UTF-8，字符串英文避免乱码
- **编译**: MSVC: `cl /EHsc /Fe:bin\*.exe /Fo:bin\*.obj src\*.c ws2_32.lib`
- **VS Code**: Code Runner 配合 run_c.bat 自动调用 MSVC

---

## 验证计划

1. `cl` 编译 server.c 和 client.c 无报错，exe/obj 输出到 bin/
2. VS Code 中 `Ctrl+Alt+N` 运行 server.c，底部终端显示监听状态
3. 另一个终端运行 `bin\client.exe`，选 1 连接成功
4. 客户端选 2 发消息，服务器端显示 `[Server] Client: xxx`
5. 服务器端终端直接输入消息，客户端实时显示 `[Server] xxx`
6. 客户端发 `quit`，双方正确断开，服务器继续监听