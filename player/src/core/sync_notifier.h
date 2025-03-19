//
// Created by 刘继玺 on 25-3-1.
//

#ifndef SYNC_NOTIFIER_H
#define SYNC_NOTIFIER_H

#include <condition_variable>
#include <mutex>

namespace jxplay {
class SyncNotifier{
public:
    SyncNotifier() = default;
    ~SyncNotifier() = default;

    void Notify();
    bool Wait(int timeout_in_milli_seconds = -1);
    void Reset();

private:
    std::mutex mutex_;
    std::condition_variable condition_var_;
    std::atomic<bool> triggered_ = false;
    bool manual_reset_ = false;

};
}

#endif //SYNC_NOTIFIER_H
