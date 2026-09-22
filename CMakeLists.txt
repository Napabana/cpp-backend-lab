cmake_minimum_required(VERSION 3.16)
project(cpp_backend_lab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 固定版本，避免上游更新导致同一份代码在不同时间得到不同构建结果。
FetchContent_Declare(
    cpp_httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.56.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(cpp_httplib)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
)
FetchContent_MakeAvailable(nlohmann_json)

find_package(Threads REQUIRED)

add_executable(thread_pool_demo
    src/main.cpp
    src/thread_pool.cpp
)

target_include_directories(thread_pool_demo PRIVATE include)
target_link_libraries(thread_pool_demo PRIVATE Threads::Threads)

add_executable(http_server_demo
    src/http_server_main.cpp
    src/http_server.cpp
)

target_include_directories(http_server_demo PRIVATE include)
target_link_libraries(http_server_demo PRIVATE
    httplib::httplib
    nlohmann_json::nlohmann_json
)
