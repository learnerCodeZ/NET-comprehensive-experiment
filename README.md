# 计算机网络综合实验 - TCP Client/Server

26.6 计算机网络课程综合实验，基于 Windows Winsock2 实现 TCP 协议的网络通信程序。

## 实验目标

使用 C 语言实现 client/server 交互式网络通信程序，熟悉 TCP 协议、Winsock2 编程接口和多线程并发处理。

## 功能特性

- 基于 **TCP 协议**（`SOCK_STREAM`）的可靠传输
- **多线程**实现双向实时对话（服务器/客户端都能主动发消息）
- 客户端菜单交互（连接/发送/断开/退出）
- 服务器显示客户端 IP 地址
- `quit` 优雅断开，服务器持续监听

## 项目结构

```
.
├── src/
│   ├── server.c    # TCP 服务器（多线程双向通信）
│   └── client.c    # TCP 客户端（菜单 + 接收线程）
├── bin/            # 编译产物（exe）
├── docs/
│   ├── demo.md     # 演示步骤
│   ├── defense.md  # 答辩预测问题
│   ├── notes.md    # 知识笔记
│   ├── usage.md    # 使用方法
│   ├── plan.md     # 实施计划
│   └── requirements.md  # 需求文档
├── .vscode/        # VS Code 配置（编译脚本）
├── README.md
└── AGENTS.md
```

## 编译运行

```bash
# 使用 MSVC
call "D:\VS2022\VC\Auxiliary\Build\vcvarsall.bat" x64
cl /EHsc /Fe:bin\server.exe src\server.c ws2_32.lib
cl /EHsc /Fe:bin\client.exe src\client.c ws2_32.lib

# 运行
bin\server.exe   # 服务器先启动
bin\client.exe   # 客户端后启动
```

详细步骤见 [docs/usage.md](docs/usage.md)。

## 课程关联

《计算机网络》第六单元（运输层），重点实践：
- TCP 面向连接传输与三次握手
- socket 编程接口（bind/listen/accept/connect/send/recv）
- 多线程并发处理
- client/server 交互模式

## 实验信息

- **课程**：计算机网络
- **适用专业**：物联网工程