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
    using Job = std::function<R(const J&)>;
    WorkerThread(Job job);
    ~WorkerThread();

    void Start();
    void Stop();

    void AddJob(const J& job);
    bool GetResult(R& result);
    size_t GetQueueSize();

    private:
    void Worker();

    std::thread worker_thread;

    std::queue<J> jobs_queue;
    std::queue<R> results_queue;
    std::mutex queues_mutex;

    std::condition_variable queue_cv;

    std::atomic<bool> running;

    // The last result
    std::optional<R> last_result;

    Job job;
};

#include "worker_thread.tpp"

#endif //OPENGUARD_WORKER_THREAD_HPP
