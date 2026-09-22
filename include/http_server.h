#pragma once

#include <httplib.h>

#include <mutex>
#include <string>
#include <unordered_map>

// P2-4：一个任务在内存中的最小数据表示。
// TODO(P2-4-1)：由你补全字段：id、type、payload、status。
struct Task {
};

// Task Service 的最小 HTTP 入口。
class HttpServer {
public:
    HttpServer();

    // 启动监听；服务器停止时返回 true，启动失败时返回 false。
    bool run();

private:
    // httplib::Server 负责监听连接、解析 HTTP，并把请求分发给匹配的 handler。
    httplib::Server server_;

    // TODO(P2-4-2)：增加一个 unordered_map<int, Task>，让 Task 在一次请求结束后仍保存在 HttpServer 对象中。
    // TODO(P2-4-3)：增加一个从 1 开始的 next_task_id_，给每个新任务分配唯一 id。
    // TODO(P2-4-4)：增加一个 mutex，保护 TaskStore 和 next_task_id_ 的并发访问。
};
