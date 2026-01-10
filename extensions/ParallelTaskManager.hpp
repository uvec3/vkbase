#pragma once

#include <thread>
#include <functional>
#include <deque>
#include <atomic>
#include <stdexcept>
#include <utility>

#include "../core/EngineBase.h"

/**
 * @brief Represents a single parallel task container.
 *
 * Holds the thread running the task, the completion finisher callback,
 * the produced data of type T and a finished flag.
 *
 * Template parameter:
 * - T: the type produced by the background task and consumed by the finisher.
 */
template<typename T>
struct ParallelTask
{
    std::thread thread;

    /**
     * Called on the main thread (or the finishing context) when the task finished.
     * Accepts the produced value of type T.
     */
    std::function<void(T)> finisher;

    /**
     * Storage for the task result. Valid after `finished` becomes true.
     */
    T data;

    /**
     * Set to true by the worker thread when the task completed.
     * Accessed concurrently by the manager, so it is atomic.
     */
    std::atomic<bool> finished{false};
};

template<typename T>
struct QueuedTask
{
    std::function<T()> task;
    std::function<void(T)> taskFinisher;
};

/**
 * @brief A small thread pool / task manager for running background tasks that
 * produce a value of type T and later call a finisher on the main thread.
 *
 * Contract:
 * - runTask() accepts two callables: a background producer returning T and a
 *   finisher receiving T which is executed after the background thread joins.
 * - finish() joins completed tasks from the back of the queue and invokes
 *   their finisher callbacks in the calling thread.
 *
 * Parameters:
 * - maxThreads: maximum number of concurrent background threads.
 * - acceptThreads: number of enqueued-but-not-yet-joined tasks that allowed to be finished
 *  if more then acceptThreads tasks added they will replace last element in the queue
 *  thus queue will never grow bigger than acceptThreads though number of threads still running can reach up to
 *  maxThreads limit only acceptThreads of them can be actually finished
 *
 * Error modes:
 * - constructor throws std::runtime_error if acceptThreads > maxThreads.
 *
 * Thread-safety and usage notes:
 * - The manager uses atomic counters/flags for simple synchronization but does
 *   not provide full concurrent modification safety for the task deque; the
 *   current usage pattern assumes a single producer (caller of runTask) and a
 *   single consumer (caller of finish / destructor).
 */
template<typename T>
class ParallelTaskManager
{
protected:
    int maxThreads=1;
    int acceptThreads=1;
    std::atomic<int> runningThreads=0;
    std::deque<ParallelTask<T>> running_tasks;
    std::deque<QueuedTask<T>> queued_tasks;
    int queueSize=1;

public:
    /**
     * @brief Construct a ParallelTaskManager.
     * @param maxThreads allowed number of threads to use
     * @param acceptThreads Number of threads to keep track of and finish
     * @throws std::runtime_error if acceptThreads > maxThreads.
     */
    explicit ParallelTaskManager(int maxThreads=1, int acceptThreads=1, int queue_size=1):
    maxThreads(maxThreads),acceptThreads(acceptThreads),queueSize(queue_size)
    {
        if(acceptThreads>maxThreads)
            throw std::runtime_error("acceptThreads>maxThreads");
    }

    /**
     * @brief Start a background task if the pool has capacity.
     *
     * The provided `task` callable will be executed in a new std::thread if
     * the number of currently running threads is less than `maxThreads`.
     * When the task completes it stores its result in the associated
     * ParallelTask::data and sets ParallelTask::finished = true. The
     * `taskFinisher` is stored and later invoked from finish() after joining
     * the worker thread.
     *
     * Inputs:
     * - task: callable returning T (executed on worker thread)
     * - taskFinisher: callable accepting T (executed by finish(), usually on main thread)
     *
     * Note: If the queue reached `acceptThreads`, the oldest queued task is
     * detached and removed to make room for the newly enqueued task.
     */
    virtual void runTask(std::function<T()> task, std::function<void(T)> taskFinisher)
    {
        if(runningThreads<maxThreads)
        {
            if(running_tasks.size()==acceptThreads)
            {
                running_tasks.back().thread.detach();
                running_tasks.pop_back();
            }

            running_tasks.emplace_front();
            ++runningThreads;
            ParallelTask<T>& currentTask = running_tasks.front();
            std::thread t([task,&currentTask,this]()
                          {
                              currentTask.data = task();
                              currentTask.finished=true;
                              --runningThreads;
                          }
            );

            currentTask.finisher=taskFinisher;
            currentTask.thread=std::move(t);
            currentTask.finished=false;
        }
        else if (queueSize>0)
        {
            if (queued_tasks.size()==queueSize)
            {
                queued_tasks.pop_front();
            }
            queued_tasks.emplace_back(QueuedTask<T>{task,taskFinisher});
        }
    }

    /**
     * @brief Join completed tasks and invoke their finisher callbacks.
     *
     * This will repeatedly check the back of the task deque. For each task
     * where `finished` is true, the manager will join the worker thread,
     * call its finisher with the produced data, and remove the task from the
     * deque. The method returns when the back task is not finished or the
     * deque becomes empty.
     */
    void finish()
    {
        while(!running_tasks.empty()&&running_tasks.back().finished)
        {
            running_tasks.back().thread.join();
            running_tasks.back().finisher(std::move(running_tasks.back().data));
            running_tasks.pop_back();

            if (!queued_tasks.empty())
            {
                runTask(queued_tasks.front().task,queued_tasks.front().taskFinisher);
                queued_tasks.pop_front();
            }
        }
    }

    /**
     * @brief Destructor detaches any remaining worker threads.
     *
     * Detaching ensures the destructor won't block waiting for background
     * threads, but it also means remaining tasks may continue running after
     * the manager is destroyed. Callers should normally ensure finish() is
     * called before destruction to process results.
     */

    int get_treads_running()
    {
        return runningThreads;
    }

    virtual ~ParallelTaskManager()
    {
        for(auto& task:running_tasks)
        {
            task.thread.detach();
        }
    }
};

/**
 * @brief Auto wrapper integrating the ParallelTaskManager with the
 *        vkbase::OnLogicUpdateReceiver to automatically call finish() every
 *        logic update.
 *
 * Behavior:
 * - When a task is enqueued, the update receiver is enabled so that
 *   onUpdateLogic() will call finish() each frame.
 * - When the queue becomes empty, the receiver is disabled again.
 */
template<typename T>
class AutoParallelTaskManager:public ParallelTaskManager<T>, public vkbase::OnLogicUpdateReceiver
{
public:
    /**
     * @brief Construct an AutoParallelTaskManager.
     * @param maxThreads passed to base ParallelTaskManager.
     * @param acceptThreads passed to base ParallelTaskManager.
     * @param priority Priority used when registering with OnLogicUpdateReceiver.
     */
    explicit AutoParallelTaskManager(int maxThreads=1, int acceptThreads=1, int priority=0):ParallelTaskManager<T>(maxThreads,acceptThreads)
    {
        vkbase::OnLogicUpdateReceiver::disable();
        vkbase::OnLogicUpdateReceiver::setPriority(priority);
    }

    /**
     * @brief Enqueue a task and ensure periodic finish() calls occur via the
     *        OnLogicUpdateReceiver.
     *
     * Overrides the base method to enable the update receiver when there are
     * tasks pending so that onUpdateLogic() will process completed work.
     */
    void runTask(std::function<T()> task, std::function<void(T)> taskFinisher) override
    {
        ParallelTaskManager<T>::runTask(task, taskFinisher);
        if(this->running_tasks.size()>0)
            vkbase::OnLogicUpdateReceiver::enable();
    }

    /**
     * @brief Called by vkbase each logic update; joins finished tasks and
     *        optionally disables the receiver when no tasks remain.
     */
    void onUpdateLogic(uint32_t imageIndex) override
    {
        this->finish();
        if(this->running_tasks.size()==0)
            vkbase::OnLogicUpdateReceiver::disable();
    }

    virtual ~AutoParallelTaskManager() = default;
};