#include "http_server.h"

#include <iostream>

int main() {
    HttpServer server;

    if (!server.run()) {
        std::cerr << "failed to listen on 127.0.0.1:8080\n";
        return 1;
    }

    return 0;
}
