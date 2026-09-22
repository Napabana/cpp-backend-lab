#include "thread_pool.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// 提交模拟任务并验证线程池的并发数、执行次数和安全退出。
int main() {
    constexpr int worker_count = 4; // 固定 worker 数量。
    constexpr int task_count = 10; // 验收任务总数。

    std::mutex result_mutex; // 保护以下验收统计状态。
    std::vector<int> execution_counts(task_count, 0); // 每个任务的执行次数。
    int active_tasks = 0; // 当前正在执行的任务数。
    int max_active_tasks = 0; // 运行期间的最大并发数。
    int completed_tasks = 0; // 已完成的任务数。

    {
        ThreadPool pool(worker_count); // 被验收的线程池。

        // 留出空闲时间，观察 worker 是否阻塞等待。
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        for (int task_id = 0; task_id < task_count; ++task_id) {
            pool.submit([&, task_id] {
                {
                    std::lock_guard<std::mutex> lock(result_mutex); // 更新开始状态。
                    ++active_tasks;
                    max_active_tasks = std::max(max_active_tasks, active_tasks);
                    std::cout << "task " << task_id + 1
                              << " started on thread " << std::this_thread::get_id()
                              << ", active: " << active_tasks << '\n';
                }

                // 模拟耗时任务，期间不持有统计锁。
                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                {
                    std::lock_guard<std::mutex> lock(result_mutex); // 更新完成状态。
                    --active_tasks;
                    ++completed_tasks;
                    ++execution_counts[task_id];
                    std::cout << "task " << task_id + 1
                              << " finished, active: " << active_tasks << '\n';
                }
            });
        }

        std::cout << "main submitted " << task_count << " tasks\n";
    }

    // 验证每个任务恰好执行一次。
    const bool every_task_ran_once = std::all_of(
        execution_counts.begin(),
        execution_counts.end(),
        [](int count) { return count == 1; }
    );

    // 汇总全部验收条件。
    const bool passed =
        completed_tasks == task_count &&
        active_tasks == 0 &&
        max_active_tasks > 1 &&
        max_active_tasks <= worker_count &&
        every_task_ran_once;

    std::cout << "\nStep 8 summary\n"
              << "completed tasks: " << completed_tasks << '/' << task_count << '\n'
              << "maximum concurrent tasks: " << max_active_tasks << '\n'
              << "every task ran once: "
              << (every_task_ran_once ? "yes" : "no") << '\n'
              << "result: " << (passed ? "PASS" : "FAIL") << '\n';

    return passed ? 0 : 1;
}
