#pragma once

#include <httplib.h>

// Task Service 的最小 HTTP 入口。
class HttpServer {
public:
    HttpServer();

    // 启动监听；服务器停止时返回 true，启动失败时返回 false。
    bool run();

private:
    // httplib::Server 负责监听连接、解析 HTTP，并把请求分发给匹配的 handler。
    httplib::Server server_;
};
