//
// Created by 刘继玺 on 25-3-1.
//

#include "sync_notifier.h"

#include <functional>

namespace jxplay {

void SyncNotifier::Notify() {
    std::unique_lock<std::mutex> lock(mutex_);
    triggered_ = true;
    condition_var_.notify_all();
}

bool SyncNotifier::Wait(int timeout_in_milli_seconds) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (!triggered_) {
        if (timeout_in_milli_seconds < 0) {
           condition_var_.wait(lock, [this](){return triggered_.load();});
        }else {
            if (!condition_var_.wait_for(lock,std::chrono::milliseconds(timeout_in_milli_seconds),
                            [this](){return triggered_.load();})) {
                return false;
            }
        }
    }
    Reset();
    return true;
}

void SyncNotifier::Reset() {
    triggered_ = false;
}

}
