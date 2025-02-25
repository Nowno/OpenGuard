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
    jobs.push(job);

    queue_cv.notify_one();
}

template <typename J, typename R>
bool WorkerThread<J, R>::GetResult(R& result)
{
    std::lock_guard<std::mutex> lock(queues_mutex);

    if (results.empty())
        return false;

    result = results.front();
    results.pop();

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

            queue_cv.wait(lock, [this] { return !jobs.empty() || !running; });

            if (!running)
                break;

            job = jobs.front();
            jobs.pop();
        }

        R result = this->job(input_job);

        {
            std::lock_guard<std::mutex> lock(queues_mutex);
            results.push(result);
        }
    }
}

#endif //OPENGUARD_WORKER_THREAD_TPP