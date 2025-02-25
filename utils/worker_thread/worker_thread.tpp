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

template <typename J, typename R>
void WorkerThread<J, R>::Start()
{
    running = true;
    worker_thread = std::thread(&WorkerThread::Worker, this);
}

template <typename J, typename R>
void WorkerThread<J, R>::Stop()
{
    running = false;

    queue_cv.notify_all();

    if (worker_thread.joinable())
    {
        worker_thread.join();
    }
}


template <typename J, typename R>
void WorkerThread<J, R>::AddJob(const J& job)
{
    std::lock_guard<std::mutex> lock(queues_mutex);
    jobs_queue.push(job);

    queue_cv.notify_one();
}

template <typename J, typename R>
bool WorkerThread<J, R>::GetResult(R& result)
{
    std::lock_guard<std::mutex> lock(queues_mutex);

    if (results_queue.empty())
        return false;

    result = results_queue.front();
    results_queue.pop();

    return true;
}

template <typename J, typename R>
void WorkerThread<J, R>::Worker()
{
    while (running)
    {
        J input_job;
        {
            std::unique_lock<std::mutex> lock(queues_mutex);

            queue_cv.wait(lock, [this] { return !jobs_queue.empty() || !running; });

            if (!running)
                break;

            input_job = jobs_queue.front();
            jobs_queue.pop();
        }

        R result = this->job(input_job);

        {
            std::lock_guard<std::mutex> lock(queues_mutex);
            results_queue.push(result);
        }

        //Sleep for a bit to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

template <typename J, typename R>
size_t WorkerThread<J, R>::GetQueueSize()
{
    std::lock_guard<std::mutex> lock(queues_mutex);
    return jobs_queue.size();
}


#endif //OPENGUARD_WORKER_THREAD_TPP