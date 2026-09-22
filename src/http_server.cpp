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

            const nlohmann::json result = {
                {"type", input.at("type").get<std::string>()},
                {"payload", input.at("payload").get<std::string>()},
                {"status", "pending"}
            };

            // 创建资源使用 201；当前只是返回结果，还没有持久化 Task。
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
