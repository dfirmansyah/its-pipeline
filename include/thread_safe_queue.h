#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <chrono>
#include <stdexcept>

using namespace std;

/**
 * @class ThreadSafeQueue
 * @brief Thread-safe data structure for thread boundary crossing
 * 
 * This queue is internal to elements and ensures safe data transfer between threads.
 */
template <typename T>
class ThreadSafeQueue {
private:
    queue<T> q;
    mutex m;
    condition_variable cv;
    bool stop_flag = false; 

public:
    void push(T value) {
        lock_guard<mutex> lock(m);
        q.push(std::move(value));
        cv.notify_one();
    }

    bool wait_and_pop(T& value) {
        unique_lock<mutex> lock(m);
        // Wait until queue is not empty OR the stop flag is set
        cv.wait(lock, [this] { return !q.empty() || stop_flag; });

        if (stop_flag && q.empty()) {
            return false; // Exit condition
        }
        
        value = std::move(q.front());
        q.pop();
        return true;
    }

    void stop() {
        {
            lock_guard<mutex> lock(m);
            stop_flag = true;
        }
        cv.notify_all(); // Wake up any waiting threads
    }
};

#endif // THREAD_SAFE_QUEUE_H