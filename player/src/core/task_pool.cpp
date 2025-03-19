//
// Created by 刘继玺 on 25-3-2.
//

#include "task_pool.h"

namespace jxplay {
TaskPool::TaskPool() {
    thread_ = std::thread([this]() { this->ThreadLoop(); });
}

TaskPool::~TaskPool() {
    // 终止任务并等待线程退出
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stop_flag_= true;
    }
    condition_var_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void TaskPool::SubmitTask(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        task_queue_.push(task);
    }
    condition_var_.notify_one();
}

void TaskPool::ThreadLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_var_.wait(lock, [this]() { return !task_queue_.empty() || stop_flag_; });

            if (stop_flag_ && task_queue_.empty()) {
                break;
            }

            task = task_queue_.front();
            task_queue_.pop();
        }
        task();
    }
}
}