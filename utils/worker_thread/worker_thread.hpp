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

    /**
     * @brief Start the worker thread.
     */
    void Start();

    /**
     * @brief Stop the worker thread.
     */
    void Stop();

    /**
     * @brief Add a job to the worker thread.
     * @param job The job to add.
     */
    void AddJob(const J& job);

    /**
     * @brief Get the result of the worker thread.
     * @param result The result to get.
     * @return Whether the result was successfully retrieved or not.
     */
    bool GetResult(R& result);

    /**
     * @brief Get the size of the job queue.
     * @return The size of the job queue.
     */
    size_t GetQueueSize();

    private:

    /**
     * @brief The worker function, that'll continuously process jobs.
     */
    void Worker();

    std::thread worker_thread;

    std::queue<J> jobs_queue;
    std::queue<R> results_queue;
    std::mutex queues_mutex;

    std::condition_variable queue_cv; /// Condition variable for the job queue
    std::atomic<bool> running;        /// Whether the worker thread is running or not
    std::optional<R> last_result;     /// The last result of the worker thread

    Job job;
};

#include "worker_thread.tpp"

#endif //OPENGUARD_WORKER_THREAD_HPP
