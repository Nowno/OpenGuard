#ifndef OPENGUARD_WORKER_THREAD_HPP
#define OPENGUARD_WORKER_THREAD_HPP

#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

template <typename J, typename R>
class WorkerThread
{
    public:
    using Job = std::function<void()>;
    WorkerThread(Job job);
    ~WorkerThread();

    void Start();
    void Stop();

    void AddJob(const J& job);
    bool GetResult(R& result);

    private:
    void Worker();

    std::thread worker_thread;

    std::queue<J> jobs;
    std::queue<R> results;
    std::mutex queues_mutex;

    std::condition_variable queue_cv;

    std::atomic<bool> running;

    Job job;
};

#include "worker_thread.tpp"

#endif //OPENGUARD_WORKER_THREAD_HPP
