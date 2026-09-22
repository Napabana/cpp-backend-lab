# C++ Backend Lab

## 项目定位

这是一个用于补齐 C++ 服务端基础的小型实验仓库，不以堆叠技术栈或一次性完成大型系统为目标。

项目最终希望把以下知识真正连接起来：

- C++ 基础与资源管理
- 多线程与线程池
- HTTP 请求处理
- MySQL 持久化
- Redis 缓存
- 消息队列
- Linux TCP / Socket
- 可选的 epoll / Reactor
- 可选的简单 Benchmark

学习目标不是“记住 API”，而是能够回答：

1. 这个组件要解决什么问题？
2. 如果没有它，会出现什么问题？
3. 这个问题在代码中由哪一部分解决？
4. 程序运行时可以如何验证？
5. 面试时能否从业务流程一路解释到具体实现？

## 最终业务场景

最终实现一个小型异步任务服务。

客户端提交任务：

```text
POST /tasks
```

目标流程：

```text
Client
  |
  v
C++ HTTP Server
  |
  v
MySQL: 保存任务，status = pending
  |
  v
Message Queue
  |
  v
Worker / ThreadPool
  |
  v
执行任务
  |
  v
MySQL: status = finished
```

客户端查询任务：

```text
GET /tasks/{id}
```

查询流程：

```text
Client
  |
  v
C++ HTTP Server
  |
  v
Redis
  |-- hit  -> return
  |
  `-- miss -> MySQL -> 写回 Redis -> return
```

## 为什么从 ThreadPool 开始

在最终系统中，HTTP Server 不应该直接同步执行所有耗时任务。

假设客户端连续提交多个独立任务，如果所有任务都在请求线程中串行执行：

```text
request -> execute task -> finish -> next request
```

一个耗时任务就会长期占用执行线程。

另一种直接做法是“每来一个任务就创建一个线程”，但线程数量会随请求量增长。请求量不可控时，这种方式缺少资源上限，也会带来更多线程创建、销毁和调度成本。

因此本项目首先实现一个固定大小的线程池：

```text
                 +--> Worker 1
submit(task) --> Task Queue --> Worker 2
                 +--> Worker 3
                 +--> Worker 4
```

核心约束是：

- 任务数量可以持续增加；
- Worker 数量固定；
- 没有任务时 Worker 阻塞等待；
- 新任务到达时唤醒 Worker；
- 多个线程安全访问共享任务队列；
- 程序退出时所有 Worker 可以安全停止。

这就是 P1 的业务背景。

## 实施路线

### P0：C++ 必要语法复习

只补后续实现立即需要的部分：

- class
- 构造函数 / 析构函数
- 引用
- STL 容器
- `std::function`
- lambda
- 智能指针
- move 语义

不重新完整学习一遍 C++。

### P1：Thread Pool

实现固定数量 Worker、线程安全任务队列、任务提交、条件变量唤醒和安全退出。

重点理解：

- `std::thread`
- `std::mutex`
- `std::condition_variable`
- producer / consumer
- race condition
- worker 生命周期

详细任务见：`docs/P1-thread-pool.md`。

### P2：HTTP Server

先使用轻量 HTTP 库完成：

- GET
- POST
- 路由
- JSON Request / Response

目标是理解 HTTP 请求如何进入 handler，以及 request / response 生命周期。

P2-1 的配置、编译、启动和验证命令见：`docs/P2-http-server.md`。

### P3：MySQL

建立最小 `tasks` 表，实现：

- 创建任务
- 查询任务
- 更新任务状态
- 参数绑定
- 后续再理解连接复用与连接池

### P4：Redis

给 `GET /tasks/{id}` 增加 Cache-Aside：

```text
先查 Redis
  |
  |-- hit  -> return
  |
  `-- miss -> MySQL -> 写回 Redis + TTL -> return
```

### P5：Message Queue

只选择一种 MQ，理解：

- Producer
- Consumer
- ACK
- Retry
- Dead Letter
- 幂等

不同时学习多套 MQ。

### P6：异步 Task Service

把 HTTP、MySQL、Redis、MQ、ThreadPool 串成第一个完整服务端闭环。

### P7：TCP / Socket

暂时绕开 HTTP 库，亲手实现：

```text
socket -> bind -> listen -> accept -> recv / send
```

理解 HTTP 之下的 TCP 连接和 Linux 文件描述符。

### P8：epoll / Reactor（选做）

只有在已经理解阻塞模型的问题之后再进入。

### P9：Benchmark（选做）

可以对线程数、缓存命中率或同步 / 异步流程进行简单测试，记录：

- 吞吐
- 延迟
- CPU / 内存使用

## 开发原则

1. 不一次性生成整个项目。
2. 每一阶段先明确业务问题，再写代码。
3. 核心模块优先自己实现；第三方库只用于暂时屏蔽当前阶段之外的问题。
4. 每一阶段必须能够编译、运行和观察行为。
5. 每完成一个阶段，都要回顾对应面试问题。
6. 如果某个语法或 API 不理解，先解释它为什么在当前场景需要，再继续实现。
7. 不提前加入 Kubernetes、服务发现、分布式事务等当前目标之外的技术。

## 推荐目录

```text
cpp-backend-lab/
├── AGENTS.md
├── README.md
├── CMakeLists.txt
├── include/
│   ├── http_server.h
│   └── thread_pool.h
├── src/
│   ├── http_server.cpp
│   ├── http_server_main.cpp
│   ├── main.cpp
│   └── thread_pool.cpp
├── tests/
└── docs/
    └── P1-thread-pool.md
```

目录会随着阶段推进逐步扩展，不提前创建大量无用模块。

## 当前阶段

P0 + P1 已完成，P2-1 已验收通过，当前做 P2-2：Route / Handler / Request / Response。

本步在已有 `GET /health` 基础上增加两个教学路由：

```text
GET /hello
GET /echo/:message
```

目标是理解 Method + Path 如何匹配 route、handler 如何读取 `Request`，以及如何填写 `Response`。

当前不做：

- POST /tasks
- JSON task parsing
- ThreadPool 与 HTTP Server 集成
- MySQL
- Redis
- Message Queue
- Socket
- epoll
