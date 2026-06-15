# 使用方法

## 一、编译程序

### 方式一：MinGW-w64（推荐）

确保 `gcc` 已安装并在 PATH 中，打开命令提示符（CMD）或 PowerShell：

```bash
cd D:\MYCODE\NET_comprehensive_experiment
gcc src\server.c -o bin\server.exe -lws2_32
gcc src\client.c -o bin\client.exe -lws2_32
```

编译成功无报错，会在 `bin` 目录下生成 `server.exe` 和 `client.exe`。

### 方式二：MSVC

在 **x64 Native Tools Command Prompt for VS** 中：

```cmd
cd D:\MYCODE\NET_comprehensive_experiment
cl /EHsc /Fe:bin\server.exe src\server.c ws2_32.lib
cl /EHsc /Fe:bin\client.exe src\client.c ws2_32.lib
```

### 方式三：Visual Studio Code

1. 安装 C/C++ 扩展（如 Microsoft C/C++）
2. 打开项目文件夹
3. 安装 MinGW-w64 并配置 `c_cpp_properties.json` 中的编译器路径
4. 右键点击 `server.c` 或 `client.c` → **"编译并运行"**

> **提示**：如果编译报错 "undefined reference to socket"，确认链接了 `ws2_32` 库（`-lws2_32` 或 `ws2_32.lib`）。

---

## 二、运行程序

### 步骤 1：启动服务器

打开**第一个终端**，运行：

```bash
bin\server.exe
```

预期输出：

```
======================================
     TCP Server - Network Experiment
======================================
Listening port: 6000

[Server] Waiting for connection...
```

服务器启动后会一直监听，直到手动关闭（Ctrl+C）。

### 步骤 2：启动客户端

打开**第二个终端**，运行：

```bash
bin\client.exe
```

预期输出：

```
======================================
     TCP Client - Network Experiment
======================================

======================================
     TCP Client - Network Experiment
======================================
  1. Connect to server
  2. Send message
  3. Disconnect
  4. Quit
======================================
Choice:
```

### 步骤 3：建立连接

在客户端输入 `1`（连接服务器），按回车：

```
Choice: 1
[Client] Connecting to 127.0.0.1:6000...
[Client] Connected successfully!
Welcome! You are connected to TCP Server.
```

此时服务器端会显示：

```
[Server] Client connected from: 127.0.0.1:xxxx
```

### 步骤 4：双向消息传递

连接建立后，客户端显示菜单，输入 `2` 发送消息：

```
Choice: 2
You: Hello Server!
[Client] Sent 13 bytes.
[Server] ACK: Hello Server!
```

服务器端同步显示：

```
[Server] Received: Hello Server!
```

双方可以反复发送消息进行对话。

### 步骤 5：断开连接

客户端输入 `2` → 输入 `quit`：

```
Choice: 2
You: quit
[Client] Sent 4 bytes.
```

服务器端：

```
[Server] Received: quit
[Server] Client requested to quit.
[Server] Waiting for next connection...
```

服务器继续监听，可接受下一个客户端连接。

客户端输入 `4` 退出程序：

```
Choice: 4
[Client] Goodbye!
```

---

## 三、客户端菜单说明

| 选项 | 功能 |
|------|------|
| 1. Connect to server | 连接服务器（默认 127.0.0.1:6000） |
| 2. Send message | 发送消息，并接收服务器回复 |
| 3. Disconnect | 主动断开当前连接 |
| 4. Quit | 断开连接并退出程序 |

---

## 四、常见问题

**Q: 编译报错 "cannot find -lws2_32"**
A: 确认 MinGW 已正确安装，或尝试使用 MSVC 编译。

**Q: 客户端连接失败 "connect failed"**
A: 确认服务器已先启动，且防火墙未阻止端口 6000。

**Q: 服务器显示 "Waiting for connection..." 后无响应**
A: 确认客户端已启动并选择连接，连接成功前服务器会一直等待。

**Q: 程序窗口闪退**
A: 从命令行运行，不要双击 exe 文件，或在代码末尾 `return 0;` 前添加 `system("pause");`。

---

## 五、运行效果示例

```
# 服务器端
[Server] Waiting for connection...
[Server] Client connected from: 127.0.0.1:52341
[Server] Received: Hello Server!
[Server] Received: quit
[Server] Client requested to quit.
[Server] Waiting for next connection...

# 客户端
1. Connect to server
2. Send message
3. Disconnect
4. Quit
Choice: 1
[Client] Connected successfully!
Choice: 2
You: Hello Server!
[Server] ACK: Hello Server!
Choice: 2
You: quit
[Client] Disconnecting...
Choice: 4
[Client] Goodbye!
```