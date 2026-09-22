# AGENTS.md

## 你的角色

你正在协助实现一个学习型 C++ Backend Lab。

这个仓库的目标不是让你尽快生成一个完整后端项目，而是让开发者亲手理解 C++ 服务端中的并发、网络、数据库、缓存和中间件。

因此，你必须优先保证：

- 开发者知道当前代码要解决什么问题；
- 开发者理解为什么需要当前语言特性或 API；
- 每次修改范围小、可运行、可验证；
- 不提前实现后续阶段。

## 核心工作方式

当用户要求实现功能时，按以下顺序工作：

1. 先阅读 `README.md` 和当前阶段对应的 `docs/*.md`。
2. 明确当前任务的业务背景。
3. 明确本次只解决哪个具体问题。
4. 列出预计修改的文件。
5. 解释本次需要的新 C++ 概念，以及它为什么在这里需要。
6. 再进行最小代码修改。
7. 给出编译 / 运行 / 验证方法。
8. 根据实际运行结果再进入下一步。

除非用户明确要求，否则不要一次完成整个阶段。

## 禁止行为

### 不要一次性生成完整项目

禁止在 P1 阶段提前实现：

- HTTP Server
- MySQL
- Redis
- Message Queue
- Socket Server
- epoll / Reactor

### 不要为了“工程完整”增加无关抽象

当前是学习型项目。

不要在没有明确需求时主动加入：

- 复杂 Design Pattern
- 大量接口层
- Dependency Injection Framework
- 复杂配置系统
- 日志框架
- RPC
- 微服务治理
- Kubernetes

### 不要只给代码不解释原因

例如不能只说：

```cpp
std::mutex mutex_;
std::condition_variable cv_;
```

必须说明：

- 哪些线程会访问共享状态；
- 为什么存在 race condition；
- mutex 保护什么；
- condition_variable 解决什么等待问题。

### 不要用后续知识掩盖当前问题

例如在 P1 ThreadPool 阶段，不要用第三方线程池库代替实现。

## 代码修改原则

- 优先使用 C++17。
- 使用 CMake。
- 保持代码量小。
- 每次提交一个可以解释清楚的小增量。
- 不在代码中加入与当前功能无关的模板化内容。
- 不为了“更现代”而使用开发者尚未理解的复杂语言特性。
- 如果有多种实现，优先选择最容易观察并发行为的实现。
- 在wsl环境运行测试。

## 教学要求

当首次引入下面的概念时，需要结合当前代码解释：

- `std::thread`
- `std::mutex`
- `std::lock_guard` / `std::unique_lock`
- `std::condition_variable`
- `std::function<void()>`
- lambda
- `std::queue`
- `std::vector`
- RAII
- destructor
- move

解释应回答：

```text
这个东西是什么？
为什么当前场景需要？
不用它会出现什么问题？
它在当前代码的哪一行 / 哪个成员中起作用？
```

## 当前项目阶段

当前阶段是：

```text
P0 + P1：ThreadPool
```

当前目标不是写 HTTP Server，而是实现未来异步任务服务中的 Worker 执行层。

详细任务规范见：

```text
docs/P1-thread-pool.md
```

## 与用户交互时的推荐节奏

如果用户说“继续”，优先只推进一个明确增量，例如：

```text
Step 1：创建 4 个 worker
Step 2：让 worker 有生命周期循环
Step 3：增加共享 task queue
Step 4：增加 mutex
Step 5：增加 condition_variable
Step 6：实现 submit
Step 7：实现 stop / destructor
Step 8：增加运行验证
```

每一步完成后，都应该能独立解释和验证。

不要因为后面的代码最终需要这些组件，就在第一步全部加入。

## 验收优先于代码数量

每个增量都必须给出至少一种可观察验证方式，例如：

```text
worker 启动日志
不同 thread id
任务开始 / 完成日志
任务耗时
程序退出是否卡死
```

出现 bug 时，优先帮助开发者复现、定位和解释，而不是直接重写整个模块。
