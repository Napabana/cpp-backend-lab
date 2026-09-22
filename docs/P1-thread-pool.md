# P1：ThreadPool 任务说明

## 1. 任务背景

最终项目是一个异步任务服务。

客户端未来会通过：

```text
POST /tasks
```

提交任务。

任务可能需要一定执行时间，例如：

- 生成报告
- 处理文件
- 执行计算
- 运行一个后台作业

如果 HTTP 请求线程直接同步执行任务：

```text
收到请求
  |
  v
执行耗时任务
  |
  v
任务完成
  |
  v
返回 Response
```

那么任务执行期间，该线程一直被占用。

如果改成每个任务都直接创建一个新线程：

```text
task 1 -> thread 1
task 2 -> thread 2
task 3 -> thread 3
...
```

线程数量就会随着任务数量增长，缺少稳定的资源上限。

因此需要一个固定大小的线程池。

## 2. 本阶段在最终系统中的位置

当前先把 HTTP、MySQL、Redis 和 MQ 全部拿掉，只保留执行层：

```text
main()
  |
  | submit(task)
  v
+----------------------+
|      ThreadPool      |
|                      |
|     Task Queue       |
|         |            |
|   +-----+-----+      |
|   |     |     |      |
|   v     v     v      |
| W1    W2    W3 ...   |
+----------------------+
```

`main()` 暂时模拟未来的 HTTP Handler / Producer。

ThreadPool 模拟未来真正执行异步任务的 Worker 层。

## 3. 最终任务目标

实现一个固定大小的 C++17 ThreadPool。

它需要满足：

1. 构造时创建固定数量 Worker。
2. Worker 创建后长期存在。
3. 主线程可以持续提交任务。
4. 任务首先进入共享队列。
5. 空闲 Worker 从队列中取任务执行。
6. 多个线程访问任务队列时不能产生数据竞争。
7. 队列为空时 Worker 应阻塞等待，而不是空转占用 CPU。
8. 新任务加入时应唤醒 Worker。
9. ThreadPool 销毁时，所有 Worker 必须能够安全退出。
10. 主程序不能因为 Worker 未结束而异常终止或永久卡住。

## 4. 第一版任务模型

第一版不处理返回值。

统一把任务表示成：

```cpp
std::function<void()>
```

因此测试任务可以是：

```cpp
[] {
    // do something
}
```

后续再考虑带参数、返回值、`future` 等能力。

## 5. 预计核心状态

最终 ThreadPool 大致需要维护以下信息，但不要一次性全部实现：

```text
workers
    保存固定数量 worker thread

tasks
    保存尚未执行的 task

mutex
    保护共享 task queue / stop state

condition_variable
    worker 没有任务时等待；新任务到达时被唤醒

stop flag
    通知 worker 线程池正在退出
```

这些成员必须随着具体问题逐步引入，而不是先照模板全部复制。

## 6. 推荐实现顺序

### Step 1：创建固定数量 Worker

目标：

```text
ThreadPool pool(4)
```

能够创建 4 个线程。

暂时不需要任务队列。

验证：打印不同 worker 的 thread id，并确保程序能够正常结束。

需要理解：

- `std::thread`
- `std::vector`
- lambda
- 为什么 thread 必须 `join()` 或 `detach()`

---

### Step 2：让 Worker 保持存活

目标：Worker 不应该创建后立即退出，而应该具备持续等待任务的生命周期。

此时先思考：

```text
while (?) {
    // wait for work
}
```

问题：

- 循环什么时候结束？
- 没任务时如果一直循环，会发生什么？

这里先明确问题，不急着一次实现最终等待机制。

---

### Step 3：增加共享 Task Queue

目标：主线程能够把多个任务放入一个公共队列。

需要理解：

- `std::queue`
- `std::function<void()>`
- Producer / Consumer

当前角色：

```text
main thread = producer
worker      = consumer
```

---

### Step 4：解决并发访问 Queue 的竞态

问题：

main thread 可能正在 `push`：

```text
tasks.push(...)
```

同时 worker 可能正在：

```text
tasks.front()
tasks.pop()
```

多个 worker 之间也可能同时操作队列。

目标：保证共享队列操作是线程安全的。

需要理解：

- race condition
- critical section
- `std::mutex`
- `std::lock_guard`
- `std::unique_lock`

重点不是“会用 mutex”，而是明确 mutex 保护的共享状态是什么。

---

### Step 5：解决 Worker 空转

错误的初始思路可能是：

```cpp
while (!stop) {
    if (!tasks.empty()) {
        // execute
    }
}
```

当队列为空时，这个循环仍会持续运行并占用 CPU。

目标：

```text
没有任务 -> worker 阻塞
有新任务 -> 唤醒 worker
```

需要理解：

- `std::condition_variable`
- `wait`
- `notify_one`
- predicate
- spurious wakeup 的基本含义

---

### Step 6：实现 submit()

目标接口：

```cpp
pool.submit(task);
```

其逻辑应包含：

```text
获得锁
  |
  v
任务进入 queue
  |
  v
释放锁
  |
  v
通知一个 worker
```

需要思考：

- 为什么修改共享队列时需要锁？
- 为什么执行任务时不应该一直持有队列 mutex？
- 为什么通常先完成队列修改，再通知 worker？

---

### Step 7：安全停止

ThreadPool 生命周期结束时：

```text
main() 即将退出
      |
      v
ThreadPool destructor
      |
      v
通知 workers 停止
      |
      v
唤醒仍在等待的 worker
      |
      v
join 所有 worker
```

需要定义第一版语义：

```text
停止接收新任务；
已经进入队列的任务执行完成后，再退出 worker。
```

目标是理解：

- destructor
- RAII
- stop flag
- `notify_all`
- `join`

---

### Step 8：运行验证

在 `main.cpp` 中提交约 10 个模拟任务。

每个任务可以：

```text
打印 task id
打印 thread id
sleep 一小段时间
打印 finish
```

期望观察：

- 4 个 worker 同时存在；
- 多个任务可以并发执行；
- 同一时刻运行的任务数不会无限增长；
- 所有任务最终完成；
- 程序可以正常退出；
- 没任务时不会出现明显 CPU 空转。

## 7. P1 完成后的验收问题

完成以后，需要能够独立解释下面的问题。

### 线程池为什么存在？

能够从“固定执行资源 + 任务排队 + 复用 Worker”解释，而不是只说“提高性能”。

### 为什么需要任务队列？

因为任务提交速度和 Worker 消费速度可能不同，需要保存尚未执行的工作。

### 为什么需要 mutex？

明确指出哪些线程共同访问哪些共享状态，以及不加锁可能发生什么。

### 为什么需要 condition_variable？

解释没有任务时 Worker 为什么需要阻塞等待，以及任务到来后如何被唤醒。

### 为什么执行 task 时不应该持有 queue mutex？

需要能够结合“其他 worker 还需要访问队列”说明。

### destructor 为什么需要 stop + notify_all + join？

能够解释等待中的 worker 如何退出，以及主线程为什么必须等待 worker 生命周期结束。

### `notify_one` 和 `notify_all` 分别在哪里使用？为什么？

能够结合提交一个任务和关闭整个线程池两个场景解释。

## 8. 当前阶段明确不做

为了控制学习范围，P1 不实现：

- `future`
- 带返回值的 `submit`
- dynamic thread pool
- work stealing
- priority queue
- lock-free queue
- coroutine
- epoll
- HTTP
- database
- Redis
- MQ

如果 Codex 主动提出这些扩展，先不做。

## 9. 推荐的第一次 Codex 指令

在仓库根目录打开 Codex 后，可以直接给：

```text
先阅读 README.md、AGENTS.md 和 docs/P1-thread-pool.md。

我们现在只做 P1 ThreadPool 的 Step 1，不要提前实现任务队列、mutex、condition_variable 或 submit。

先检查当前仓库结构，然后告诉我：
1. Step 1 的具体目标；
2. 需要修改哪些文件；
3. 我需要先理解哪些最少的 C++ 知识；
4. std::thread 在这个业务场景里具体解决什么问题。

解释完成后，再实现最小版本，并给出本地编译和验证命令。
```

完成 Step 1 并确认运行结果后，再让 Codex继续 Step 2。
