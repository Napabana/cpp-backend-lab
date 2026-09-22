# P2-1：HTTP Server Bootstrap 运行配置

本文件只记录 P2-1 的构建与运行命令。以下命令均在 WSL 中执行。

## 1. 进入项目目录

```bash
cd /mnt/e/2806/cpp-backend-lab
```

## 2. 检查构建工具

```bash
cmake --version
g++ --version
```

如果尚未安装 CMake：

```bash
sudo apt-get update
sudo apt-get install -y cmake
```

## 3. 配置项目

```bash
cmake -S . -B build-wsl-p2 -DCMAKE_BUILD_TYPE=Debug
```

CMake 会下载 `CMakeLists.txt` 中固定版本的 cpp-httplib。

## 4. 编译 HTTP Server

```bash
cmake --build build-wsl-p2 --target http_server_demo -j2
```

## 5. 启动服务器

```bash
./build-wsl-p2/http_server_demo
```

预期启动日志：

```text
HTTP server listening on http://127.0.0.1:8080
```

`listen("127.0.0.1", 8080)` 会阻塞当前终端，这是服务器正在持续等待请求的正常表现。

## 6. 验证 GET /health

保留服务器终端，在另一个 WSL 终端执行：

```bash
cd /mnt/e/2806/cpp-backend-lab
curl --noproxy '*' -i http://127.0.0.1:8080/health
```

预期响应：

```http
HTTP/1.1 200 OK
Content-Type: text/plain
Content-Length: 3

OK
```

服务器终端应同时出现：

```text
received GET /health
```

## 7. 停止服务器

回到服务器终端，按：

```text
Ctrl+C
```

本步骤不包含 POST、JSON、ThreadPool 接入或其他 P2-2 功能。
