#include "thread_pool.h"

#include <iostream>
#include <stdexcept>

// 创建固定数量的 worker，并让每个 worker 持续等待和执行任务。
ThreadPool::ThreadPool(std::size_t worker_count) {
    for (std::size_t worker_index = 0;
         worker_index < worker_count;
         ++worker_index) {
        workers_.emplace_back([this, worker_index] {
            while (true) {
                Task task = waitAndTakeTask(); // 当前 worker 取得的任务。
                if (!task) {
                    break;
                }

                std::cout << "worker " << worker_index
                          << ", thread id: " << std::this_thread::get_id()
                          << " executing task\n";

                task(); // 在队列锁之外执行耗时任务。
            }

            std::cout << "worker " << worker_index << " stopped\n";
        });
    }
}

// 发出停止信号，唤醒全部 worker，并等待它们处理完队列后退出。
ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(tasks_mutex_); // 保护 stop_。
        stop_ = true;
    }

    task_cv_.notify_all();

    for (std::thread& worker : workers_) {
        worker.join();
    }
}

// 安全地加入任务；线程池停止后拒绝新任务。
void ThreadPool::submit(Task task) {
    std::size_t pending_tasks = 0; // 记录入队后的待执行任务数。

    {
        std::lock_guard<std::mutex> lock(tasks_mutex_); // 保护队列和停止状态。
        if (stop_) {
            throw std::runtime_error("cannot add task to a stopped ThreadPool");
        }

        tasks_.push(task);
        pending_tasks = tasks_.size();
    }

    task_cv_.notify_one();
    std::cout << "pending tasks after push: " << pending_tasks << '\n';
}

// 阻塞等待任务或停止信号，并在同一临界区内取出队首任务。
ThreadPool::Task ThreadPool::waitAndTakeTask() {
    std::unique_lock<std::mutex> lock(tasks_mutex_); // wait 可临时释放的队列锁。

    task_cv_.wait(lock, [this] {
        return stop_ || !tasks_.empty();
    });

    if (stop_ && tasks_.empty()) {
        return {};
    }

    Task task = tasks_.front(); // 保存队首任务，供锁外执行。
    tasks_.pop();
    return task;
}
