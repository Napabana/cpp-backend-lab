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

    // 固定路径：只有 GET /hello 会匹配这个 handler。
    server_.Get("/hello", [](const httplib::Request& request,
                             httplib::Response& response) {
        std::cout << "received " << request.method << ' ' << request.path << '\n';

        // Response 中最核心的是 status、headers 和 body。
        // set_content 会设置 body，同时生成对应的 Content-Type。
        response.status = 200;
        response.set_content("hello\n", "text/plain");
    });

    // 动态路径：:message 表示这一段路径由客户端提供。
    // 例如 GET /echo/cpp 会得到 path_params["message"] == "cpp"。
    server_.Get("/echo/:message", [](const httplib::Request& request,
                                     httplib::Response& response) {
        const std::string& message = request.path_params.at("message");

        std::cout << "received " << request.method << ' ' << request.path
                  << ", message=" << message << '\n';

        response.status = 200;
        response.set_content(message + "\n", "text/plain");
    });
}

bool HttpServer::run() {
    std::cout << "HTTP server listening on http://127.0.0.1:8080\n";

    // listen 启动服务器并持续等待请求。
    // cpp-httplib 解析请求后，会按 Method + Path 找到对应 route，再调用 handler。
    return server_.listen("127.0.0.1", 8080);
}
