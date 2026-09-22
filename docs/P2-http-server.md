# P2 HTTP Server：学习目标、实施边界与分步验收

> 项目：`cpp-backend-lab`  
> 阶段：P2 — HTTP Server  
> 技术：C++17 + `cpp-httplib` + `nlohmann/json` + CMake + curl  
> 原则：这是学习型服务端实验，不是为了快速堆完整后端。

---

## 0. 给 Codex 的执行规则

这份文档是 **P2 的阶段说明与验收标准**，不是让你一次性完成所有 TODO。

必须遵守：

1. 用户明确说“执行 P2-X”时，只实现对应 Step。
2. 完成当前 Step 后立刻停止，不要自动进入下一 Step。
3. 修改前先说明：
   - 当前业务问题是什么；
   - 为什么这一步需要这些代码；
   - 准备修改哪些文件。
4. 修改后说明：
   - 实际修改了哪些文件；
   - 每个修改解决了什么问题；
   - 给出编译、启动、curl 验证命令。
5. 不要因为“工程上更规范”提前引入：
   - MySQL
   - Redis
   - RabbitMQ / Kafka
   - 自定义 ThreadPool
   - 手写 socket
   - epoll / Reactor
   - RPC
   - Controller / Service / Repository 分层
   - 配置中心、日志框架、依赖注入等额外架构
6. 如果发现后续 Step 的代码已经被提前加入，先指出，不要继续扩展。
7. P2 的重点是让用户能解释真实请求生命周期，而不是只让功能跑起来。

---

# 1. P2 的业务目标

最终项目会实现一个异步任务服务：

```text
POST /tasks
    ↓
HTTP Server
    ↓
MySQL(status=pending)
    ↓
Message Queue
    ↓
Worker / ThreadPool
    ↓
MySQL(status=finished)
```

查询流程最终会是：

```text
GET /tasks/{id}
    ↓
HTTP Server
    ↓
Redis
 ├─ hit  → return
 └─ miss → MySQL → 写回 Redis → return
```

但是 **P2 只学习 HTTP 层**。

P2 要回答的是：

> 客户端发出的 HTTP 请求怎样进入 C++ 程序，如何匹配路由、读取 Request、解析 JSON、执行 handler，并构造 Response？

底层：

```text
socket → bind → listen → accept → recv → send
```

暂时由 `cpp-httplib` 负责，留到 P7 手写。

---

# 2. P2 技术选择

使用：

- HTTP：`cpp-httplib`
- JSON：`nlohmann/json`
- 构建：CMake
- 手工验证：curl
- C++：C++17

依赖必须通过 CMake 固定版本。

当前确认版本：

```text
cpp-httplib: v0.56.0
nlohmann/json: v3.12.0
```

`cpp-httplib` 在当前阶段负责：

```text
监听端口
→ 接收连接
→ 解析 HTTP
→ Method + Path 路由匹配
→ 构造 Request
→ 调用 handler
→ 根据 Response 生成 HTTP 响应
```

注意：P2 不去展开这些操作内部的 socket 实现。

---

# 3. 当前仓库基线

当前已存在 P1 ThreadPool 示例，并新增了 P2 HTTP Server：

```text
CMakeLists.txt
include/http_server.h
src/http_server.cpp
src/http_server_main.cpp
```

当前 CMake 同时保留：

```text
thread_pool_demo
http_server_demo
```

这符合“P1 已完成、P2 单独学习”的阶段性组织方式。

当前暂时 **不要求把 `http_server_demo` 重命名为 `cpp_backend_lab`**。最终 P2 收口时再决定是否统一入口，避免当前为了名字产生无学习价值的改动。

---

# 4. P2-1 验收结果

## 4.1 已经做对的部分

当前实现已经完成了 P2-1 的主要骨架：

```text
main()
  ↓
构造 HttpServer
  ↓
HttpServer 构造函数中注册 GET /health
  ↓
server.run()
  ↓
httplib::Server::listen("127.0.0.1", 8080)
```

当前设计是合适的：

### `HttpServer` 只包装最小 HTTP Server

```cpp
class HttpServer {
public:
    HttpServer();
    bool run();

private:
    httplib::Server server_;
};
```

没有提前加入数据库、TaskStore、线程池或复杂业务层。

### route 在构造阶段注册

当前：

```cpp
server_.Get("/health", handler);
```

这里的关键点是：

- `Get`：HTTP Method 为 GET；
- `"/health"`：请求路径；
- lambda：路由匹配以后执行的 handler。

构造 `HttpServer` 时只是 **注册规则**，不是已经收到请求。

### `run()` 负责真正启动服务

当前：

```cpp
server_.listen("127.0.0.1", 8080);
```

调用后，当前启动线程会停留在服务器运行流程中等待请求。

### 没有提前手写网络层

这是正确边界。

P2 当前只需要理解：

```text
HTTP Request
→ route
→ handler
→ Request / Response
```

不应该在这里展开 `accept / recv / send`。

---

## 4.2 验收结论

P2-1 已由用户在本地 WSL 环境完成编译、启动与 curl 验证。

当前基线已经满足：

```text
GET /health
→ 200
→ Content-Type: application/json
→ {"status":"ok"}
```

同时：

- `cpp-httplib` 固定为 v0.56.0；
- `nlohmann/json` 固定为 v3.12.0；
- 服务端能够打印请求 method 和 path；
- 没有提前加入 TaskStore、数据库或底层 socket 实现。


---

# 5. P2-1：HTTP Server Bootstrap

## 5.1 当前业务问题

现在需要的不是“任务系统”，而只是先证明：

> 一个外部 HTTP 请求能够进入 C++ 程序，并由我们注册的 handler 生成响应。

---

## 5.2 本 Step 应具备的能力

完成后：

```bash
./build/http_server_demo
```

监听：

```text
127.0.0.1:8080
```

执行：

```bash
curl -i http://127.0.0.1:8080/health
```

能够得到：

```text
HTTP/1.1 200 ...
Content-Type: application/json
...

{"status":"ok"}
```

---

## 5.3 P2-1 允许修改的文件

优先只修改：

```text
CMakeLists.txt
src/http_server.cpp
```

除非编译确实需要，否则不要修改：

```text
include/http_server.h
src/http_server_main.cpp
```

---

## 5.4 P2-1 严禁提前实现

不要增加：

```text
GET /hello
GET /echo/...
POST /tasks
Task
TaskStore
unordered_map
mutex
数据库
缓存
消息队列
```

这些属于后续 Step。

---

## 5.5 P2-1 完成后必须解释的运行过程

用户必须能够说清：

```text
1. 进程从 main() 开始。
2. `HttpServer server;` 调用构造函数。
3. 构造函数执行 `server_.Get(...)`，把 GET + /health + handler 注册到 httplib::Server。
4. 此时还没有客户端请求，也没有执行 handler。
5. main 调用 `server.run()`。
6. run 调用 `server_.listen("127.0.0.1", 8080)`。
7. listen 启动服务器并持续等待请求，因此正常运行时不会立刻返回。
8. curl 向 127.0.0.1:8080 发 GET /health。
9. cpp-httplib 接收并解析请求。
10. 库根据 Method=GET、Path=/health 找到已注册 route。
11. 库调用对应 lambda handler，并提供 Request 和 Response。
12. handler 读取 Request 中需要的信息，并填写 Response。
13. handler 返回。
14. cpp-httplib 把 Response 序列化成 HTTP Response 发回客户端。
15. curl 显示 status、header 和 body。
16. 服务仍继续监听下一次请求。
```

这里先停在 HTTP 库抽象层。

P7 再进入：

```text
socket
bind
listen
accept
recv
send
```

---

## 5.6 P2-1 编译与验证

源码在 Windows 磁盘、编译运行在 WSL 时，可以在 WSL 进入仓库目录后执行：

```bash
cmake -S . -B build
cmake --build build -j
```

终端 A：

```bash
./build/http_server_demo
```

终端 B：

```bash
curl -i http://127.0.0.1:8080/health
```

还可以观察服务端日志：

```text
received GET /health
```

### 验收标准

必须同时满足：

- [ ] CMake 配置成功
- [ ] `http_server_demo` 编译成功
- [ ] 服务监听 `127.0.0.1:8080`
- [ ] `/health` 返回 200
- [ ] `Content-Type` 是 JSON
- [ ] body 包含 `{"status":"ok"}`
- [ ] 服务端能看到请求 method 和 path
- [ ] 没有出现 P2-2 之后的代码

完成这些以后，停止修改并等待用户确认。

---

# 6. P2-2：Route / Handler / Request / Response

> P2-1 已验收通过；当前执行本 Step。

## 6.1 业务问题

P2-1 已经证明请求能进来。

下一步要回答：

> 一个请求进入 HTTP Server 之后，库到底根据什么决定执行哪段业务代码？

核心流程：

```text
curl
 ↓
HTTP Request
 ↓
cpp-httplib
 ↓
Method + Path 匹配 route
 ↓
handler(req, res)
 ↓
读取 Request
 ↓
填写 Response
 ↓
HTTP Response
 ↓
curl
```

---

## 6.2 最小新增功能

只允许加入教学路由，例如：

```text
GET /hello
GET /echo/:message
```

`cpp-httplib` 当前支持：

```cpp
server.Get("/echo/:message", handler);
```

并可从：

```cpp
request.path_params
```

读取路径参数。

不要加入 TaskStore。

---

## 6.3 本 Step 必须理解

### HTTP Method

例如：

```text
GET
POST
```

Method 表示客户端想对资源执行哪类操作。

### Path

例如：

```text
/health
/hello
/echo/abc
```

### Route

Route 是：

```text
Method + Path Pattern + Handler
```

例如：

```text
GET + /health + health handler
```

### Handler

handler 是请求匹配成功后由 HTTP Server 调用的 callback。

当前用 lambda 表示。

### Request

`httplib::Request` 是库解析后的请求对象。

后续会逐步读取：

```text
method
path
headers
body
path_params
```

### Response

`httplib::Response` 是 handler 填写的响应对象。

重点包含：

```text
status
headers
body
Content-Type
```

---

## 6.4 P2-2 严禁提前实现

不要加入：

```text
POST /tasks
JSON request body parsing
Task
TaskStore
mutex
数据库
```

完成简单 route 教学并通过 curl 后停止。

---

# 7. P2-3：POST /tasks + JSON

> P2-2 已验收通过；当前执行本 Step。

## 7.1 业务问题

前面已经知道 route 怎样找到 handler。

现在要解决：

> 客户端怎样把结构化数据放进 HTTP Request，C++ 怎样读取，再怎样把 C++ 数据转换成 JSON Response？

流程：

```text
Client
 ↓
POST /tasks
 ↓
Request Body
 ↓
JSON parse
 ↓
C++ 数据
 ↓
JSON serialize
 ↓
Response
```

---

## 7.2 输入

```json
{
  "type": "demo",
  "payload": "hello"
}
```

---

## 7.3 暂时返回

```json
{
  "type": "demo",
  "payload": "hello",
  "status": "pending"
}
```

此时 **不要保存任务**。

下一次请求不需要还能查到。

---

## 7.4 本 Step 必须理解

- GET 与 POST 的差别
- Request Body
- `Content-Type: application/json`
- `request.body`
- `nlohmann::json::parse`
- JSON 字段读取
- `dump()`
- 字段缺失
- JSON 语法错误
- 为什么客户端非法输入返回 400

---

## 7.5 严禁提前实现

不要加入：

```text
TaskStore
unordered_map
自增 id
GET /tasks/{id}
mutex
```

---

# 8. P2-4：Task + 内存 TaskStore

> 只有 P2-3 验收完成后执行。

## 8.1 业务问题

P2-3 每个请求都是独立的。

现在要回答：

> POST 创建的数据为什么能在请求结束以后继续存在，并被后续请求读取？

最小数据结构：

```cpp
struct Task {
    int id;
    std::string type;
    std::string payload;
    std::string status;
};
```

最小存储：

```text
unordered_map<int, Task>
```

---

## 8.2 本 Step 必须理解

- Server 进程的生命周期
- `HttpServer` 对象生命周期
- TaskStore 生命周期
- 请求结束后局部变量为什么消失
- 成员对象为什么还能保留
- ID 怎样生成
- 多个 handler 是否可能同时访问共享状态
- 为什么共享 TaskStore 需要考虑互斥

这里要把 P1 的知识迁移过来：

```text
P1:
多个 worker
→ 共享 queue
→ mutex

P2:
多个 HTTP request handler
→ 共享 TaskStore
→ mutex
```

不要重新完整教学 mutex，只解释它在当前 TaskStore 场景中的作用。

---

# 9. P2-5：GET /tasks/{id}

> 只有 P2-4 验收完成后执行。

## 9.1 业务问题

现在已有持久存在于进程内存中的 TaskStore。

下一步：

> 客户端怎样通过 URL 指定某一个资源？

实现：

```text
GET /tasks/{id}
```

在 `cpp-httplib` 当前 API 中，可以使用 path parameter：

```text
/tasks/:id
```

然后读取：

```text
request.path_params["id"]
```

---

## 9.2 必须处理的情况

```text
合法 id + 找到任务
→ 200

合法 id + 任务不存在
→ 404

非法 id
→ 400
```

---

## 9.3 完整验证流程

```bash
POST /tasks
```

得到：

```json
{"id":1,...}
```

然后：

```bash
GET /tasks/1
```

应得到对应 Task。

再测试：

```bash
GET /tasks/999999
```

应得到 404。

再测试非法 id。

---

# 10. P2-6：错误处理与收口

> 只有 P2-5 验收完成后执行。

统一检查：

```text
200 OK
201 Created
400 Bad Request
404 Not Found
500 Internal Server Error
```

重点不是背状态码，而是知道：

```text
哪个错误属于客户端输入问题
哪个表示资源不存在
哪个表示服务端内部异常
```

同时统一检查：

- Content-Type
- JSON 错误格式
- 缺少字段
- 不存在资源
- HTTP Server 生命周期
- TaskStore 生命周期

---

# 11. P2 最终必须能够独立解释的请求生命周期

P2 完成后，用户应该能够从一次 curl 解释：

```text
curl
 ↓
TCP connection
 ↓
cpp-httplib
 ↓
HTTP parsing
 ↓
Method + Path route matching
 ↓
handler
 ↓
Request
 ↓
JSON parsing
 ↓
TaskStore
 ↓
Response JSON
 ↓
HTTP status + headers + body
 ↓
cpp-httplib
 ↓
客户端
```

其中：

```text
TCP connection
 ↓
socket / accept / recv / send
```

P2 只知道其存在并由库负责。

具体实现留给 P7。

---

# 12. P2 完成后的系统能力

完成 P2 后，系统应该只有：

```text
HTTP Server
+
JSON Request / Response
+
内存 TaskStore
```

支持：

```text
GET /health
POST /tasks
GET /tasks/{id}
```

此时还没有真正的数据持久化。

因此系统的自然缺口是：

```text
服务进程一退出
→ 内存 TaskStore 消失
→ Task 全部丢失
```

这正是 P3 MySQL 出现的业务原因：

> 把“只在进程活着时存在的任务状态”变成持久化数据。

因此 P2 完成后再进入 P3，而不是提前在 HTTP 学习阶段加入数据库。

---

# 13. 当前下一步

当前只执行：

```text
P2-3：POST /tasks + JSON
```

本 Step：

1. 保留已有 GET routes。
2. 增加 `POST /tasks`。
3. 从 `request.body` 读取原始请求体。
4. 使用 `nlohmann::json::parse` 解析 JSON。
5. 要求 `type`、`payload` 存在且为字符串。
6. 成功时返回 201 与 `{"type":...,"payload":...,"status":"pending"}`。
7. JSON 语法错误或字段错误返回 400。
8. 不保存任务，不加入 TaskStore、自增 ID、GET /tasks/{id} 或 mutex。
9. curl 验收成功、非法 JSON、缺失字段三种情况后停止。
