#include "http_server.h"

#include <iostream>
#include <nlohmann/json.hpp>

HttpServer::HttpServer() {
    // Route = HTTP Method + Path Pattern + Handler。
    server_.Get("/health", [](const httplib::Request& request,
                              httplib::Response& response) {
        std::cout << "received " << request.method << ' ' << request.path << '\n';

        const nlohmann::json body = {{"status", "ok"}};
        response.set_content(body.dump(), "application/json");
    });

    server_.Get("/hello", [](const httplib::Request& request,
                             httplib::Response& response) {
        std::cout << "received " << request.method << ' ' << request.path << '\n';

        response.status = 200;
        response.set_content("hello\n", "text/plain");
    });

    server_.Get("/echo/:message", [](const httplib::Request& request,
                                     httplib::Response& response) {
        const std::string& message = request.path_params.at("message");

        std::cout << "received " << request.method << ' ' << request.path
                  << ", message=" << message << '\n';

        response.status = 200;
        response.set_content(message + "\n", "text/plain");
    });

    // POST /tasks 的数据放在 Request Body 中，并使用 JSON 表示。
    // 本 Step 只做“解析 -> 校验 -> 返回”，还不会保存任务。
    // P2-4 TODO：当前仍保留 P2-3 的“解析后直接返回”实现，因此代码可以继续编译。
    // 你需要把这个 handler 改成真正创建 Task 并保存到 HttpServer 的成员 TaskStore。
    // TODO(P2-4-5)：为了访问 HttpServer 的成员，把 lambda 捕获从 [] 改成 [this]。
    server_.Post("/tasks", [](const httplib::Request& request,
                              httplib::Response& response) {
        std::cout << "received " << request.method << ' ' << request.path
                  << ", body=" << request.body << '\n';

        try {
            // request.body 仍然只是字符串；parse 后才得到可按字段访问的 JSON 对象。
            const nlohmann::json input = nlohmann::json::parse(request.body);

            // 当前接口要求 type 和 payload 都存在，并且都是字符串。
            if (!input.is_object() ||
                !input.contains("type") ||
                !input.contains("payload") ||
                !input.at("type").is_string() ||
                !input.at("payload").is_string()) {
                response.status = 400;
                const nlohmann::json error = {
                    {"error", "type and payload must be strings"}
                };
                response.set_content(error.dump(), "application/json");
                return;
            }

            // TODO(P2-4-6)：删除下面这段 P2-3 临时返回逻辑，改成：
            // 1. 构造一个 Task。
            // 2. 从 input 读取 type / payload，status 设为 "pending"。
            // 3. 用 next_task_id_ 给它分配 id。
            // 4. 用 lock_guard 锁住 TaskStore 后保存任务。
            // 5. 返回包含 id / type / payload / status 的 JSON。
            //
            // 注意：id 分配和写入 TaskStore 必须处于同一个互斥保护范围内，
            // 否则两个并发 POST 可能拿到相同 id，或同时修改容器。

            const nlohmann::json result = {
                {"type", input.at("type").get<std::string>()},
                {"payload", input.at("payload").get<std::string>()},
                {"status", "pending"}
            };

            response.status = 201;
            response.set_content(result.dump(), "application/json");
        } catch (const nlohmann::json::parse_error&) {
            // JSON 语法本身非法属于客户端请求错误。
            response.status = 400;
            const nlohmann::json error = {{"error", "invalid json"}};
            response.set_content(error.dump(), "application/json");
        }
    });
}

bool HttpServer::run() {
    std::cout << "HTTP server listening on http://127.0.0.1:8080\n";

    return server_.listen("127.0.0.1", 8080);
}
