#ifndef OPENGUARD_WORKER_THREAD_TPP
#define OPENGUARD_WORKER_THREAD_TPP

template <typename J, typename R>
WorkerThread<J, R>::WorkerThread(Job job) : job(job), running(false)
{

}


template <typename J, typename R>
WorkerThread<J, R>::~WorkerThread()
{
    Stop();
}

/**
 * @brief Start the worker thread.
 */
template <typename J, typename R>
void WorkerThread<J, R>::Start()
{
    running = true;
    worker_thread = std::thread(&WorkerThread::Worker, this);
}

/**
 * @brief Stop the worker thread.
 */
template <typename J, typename R>
void WorkerThread<J, R>::Stop()
{
    running = false;

    queue_cv.notify_all();

    /// Wait for the worker thread to finish
    if (worker_thread.joinable())
    {
        worker_thread.join();
    }
}


/**
 * @brief Add a job to the worker thread.
 */
template <typename J, typename R>
void WorkerThread<J, R>::AddJob(const J& job)
{
    std::lock_guard<std::mutex> lock(queues_mutex);
    jobs_queue.push(job);
    /// Notify the worker thread that a new job has been added
    queue_cv.notify_one();
}

/**
 * @brief Get the result of the worker thread.
 */
template <typename J, typename R>
bool WorkerThread<J, R>::GetResult(R& result)
{
    std::lock_guard<std::mutex> lock(queues_mutex);

    /// If no results are available, return false
    if (results_queue.empty())
        return false;

    /// Otherwise, update the result and assert that it was successfully retrieved
    result = results_queue.front();
    results_queue.pop();

    return true;
}

/**
 * @brief Get the size of the job queue.
 */
template <typename J, typename R>
void WorkerThread<J, R>::Worker()
{
    while (running)
    {
        J input_job;
        {
            std::unique_lock<std::mutex> lock(queues_mutex);

            /// Continuously wait for a job to be added to the queue
            queue_cv.wait(lock, [this] { return !jobs_queue.empty() || !running; });

            /// If the worker thread is no longer running, break out of the loop
            if (!running)
                break;

            /// Otherwise, get the job from the queue
            input_job = jobs_queue.front();
            jobs_queue.pop();
        }

        /// Process the job and add the result to the results queue
        R result = this->job(input_job);

        {
            std::lock_guard<std::mutex> lock(queues_mutex);
            results_queue.push(result);
        }

        /// Sleep for a bit to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

/**
 * @brief Get the size of the job queue.
 */
template <typename J, typename R>
size_t WorkerThread<J, R>::GetQueueSize()
{
    std::lock_guard<std::mutex> lock(queues_mutex);
    return jobs_queue.size();
}


#endif //OPENGUARD_WORKER_THREAD_TPP