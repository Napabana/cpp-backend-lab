#include "http_server.h"

#include <iostream>
#include <nlohmann/json.hpp>

HttpServer::HttpServer() {
    // route 是 HTTP 方法与路径的匹配规则；handler 是匹配成功后执行的函数。
    server_.Get("/health", [](const httplib::Request& request,
                              httplib::Response& response) {
        // Request 表示客户端已经被解析的请求，这里观察它的方法和路径。
        std::cout << "received " << request.method << ' ' << request.path << '\n';

        // 用 C++ 对象构造 JSON，再序列化为 Response body。
        const nlohmann::json body = {{"status", "ok"}};

        // Response 表示将要返回给客户端的响应；库会把它序列化为 HTTP 报文。
        response.set_content(body.dump(), "application/json");
    });
}

bool HttpServer::run() {
    std::cout << "HTTP server listening on http://127.0.0.1:8080\n";

    // listen 内部会持续接受并处理连接，所以服务器运行期间当前线程会阻塞在这里。
    // 调用过程：GET /health 到达 -> 匹配上面的 route -> 执行 handler -> 发送 Response。
    return server_.listen("127.0.0.1", 8080);
}
