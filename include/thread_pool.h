#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// 固定数量 worker 的任务执行池。
class ThreadPool {
public:
    // 无参数、无返回值的任务类型。
    using Task = std::function<void()>;

    // 创建 worker_count 个 worker。
    explicit ThreadPool(std::size_t worker_count);

    // 停止接收任务，处理完队列并等待所有 worker 退出。
    ~ThreadPool();

    // 将任务加入队列并唤醒一个 worker。
    void submit(Task task);

private:
    // 等待并取出一个任务；返回空 Task 表示 worker 应退出。
    Task waitAndTakeTask();

    // 保存固定数量的 worker 线程。
    std::vector<std::thread> workers_;

    // 保存尚未执行的任务。
    std::queue<Task> tasks_;

    // 保护 tasks_ 和 stop_。
    std::mutex tasks_mutex_;

    // 在任务到达或线程池停止时唤醒 worker。
    std::condition_variable task_cv_;

    // 表示线程池是否已经停止接收新任务。
    bool stop_{false};
};
