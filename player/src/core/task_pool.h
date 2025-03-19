//
// Created by 刘继玺 on 25-3-2.
//

#ifndef TASK_POOL_H
#define TASK_POOL_H

#include <iostream>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>


namespace jxplay {
class TaskPool{
public:
    TaskPool();
    ~TaskPool();
    void SubmitTask(std::function<void()> task);
private:
    void ThreadLoop();
private:
    std::thread thread_;
    std::queue<std::function<void()>> task_queue_;
    std::mutex mutex_;
    std::condition_variable condition_var_;
    std::atomic<bool> stop_flag_ = false;

};
}



#endif //TASK_POOL_H
