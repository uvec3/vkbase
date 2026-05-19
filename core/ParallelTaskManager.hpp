#pragma once

#include <thread>
#include <functional>
#include <deque>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <utility>


#include "Event.hpp"
#include "../extensions/ShadersRC/slang/slang/external/unordered_dense/include/ankerl/unordered_dense.h"


/**
 * @brief Represents a single parallel task container.
 *
 * Holds the thread running the task, the completion finisher callback,
 * the produced data of type T and a finished flag.
 *
 * Template parameter:
 * - T: the type produced by the background task and consumed by the finisher.
 */

struct IRunningTask
{
    virtual void callFinisher()=0;
    virtual void callDetachAction()=0;
    virtual ~IRunningTask()=default;

    std::jthread thread;
    std::atomic<bool> finished{false};
};

template<typename T>
struct RunningTask:IRunningTask
{
    std::move_only_function<void(T)> finisher;
    std::move_only_function<void(T)> detachAction;
    T data;

    void callFinisher() override
    {
        finisher(std::move(data));
    }

    void callDetachAction() override
    {
        detachAction(std::move(data));
    }

    RunningTask()
    {
        std::cout<<"RunningTask created\n";
    }

    //delete copy and move constructors and assignment operators
    RunningTask(const RunningTask&) = delete;
    RunningTask& operator=(const RunningTask&) = delete;
    RunningTask(RunningTask&&) = delete;
    RunningTask& operator=(RunningTask&&) = delete;



    ~RunningTask() override
    {
        if (thread.joinable())
            thread.join();
        std::cout<<"RunningTask destroyed\n";
    }
};

struct IQueuedTask
{
    virtual std::unique_ptr<IRunningTask> run(std::atomic<int>& runningThreads)=0;
    virtual ~IQueuedTask()=default;
};

template<typename T>
struct QueuedTask:IQueuedTask
{
    std::move_only_function<T()> task;
    std::move_only_function<void(T)> taskFinisher;
    std::move_only_function<void(T)> detachAction;

    std::unique_ptr<IRunningTask> run(std::atomic<int>& runningThreads) override
    {
        ++runningThreads;
        auto running_task=std::make_unique<RunningTask<T>>();
        auto fn_local = std::move(task);
        running_task->finisher=std::move(taskFinisher);
        running_task->detachAction=std::move(detachAction);
        running_task->thread = std::jthread(
            [&runningThreads,task_ptr=running_task.get(),fn=std::move(fn_local)]() mutable
        {
            task_ptr->data = fn();
            task_ptr->finished=true;
        });


        return running_task;
    };
};



class ParallelTaskManager
{
protected:
    int maxThreads=1;
    int acceptThreads=1;
    std::atomic<int> runningThreads=0;
    std::deque<std::unique_ptr<IRunningTask>> running_tasks;
    std::deque<std::unique_ptr<IQueuedTask>> queued_tasks;
    std::vector<std::unique_ptr<IRunningTask>> detached_tasks;
    int queueSize=1;

public:

    explicit ParallelTaskManager(int maxThreads=1, int acceptThreads=1, int queue_size=1):
    maxThreads(maxThreads),acceptThreads(acceptThreads),queueSize(queue_size)
    {
        if(acceptThreads>maxThreads)
            throw std::runtime_error("acceptThreads>maxThreads");
    }

    explicit ParallelTaskManager(int maxThreads=1, int queue_size=1, bool only_finish_last=false):
    maxThreads(maxThreads),queueSize(queue_size)
    {
        if (only_finish_last)
            acceptThreads=1;
        else
            acceptThreads=maxThreads;
    }

    template<typename TaskFunc, typename FinisherFunc>
    void runTask(TaskFunc&& task, FinisherFunc&& taskFinisher)
    {
        using T = std::invoke_result_t<TaskFunc>;
        runTask(std::forward<TaskFunc>(task), std::forward<FinisherFunc>(taskFinisher), [](T){});
    }

    template<typename TaskFunc, typename FinisherFunc,typename DetachAction>
    void runTask(TaskFunc&& task, FinisherFunc&& taskFinisher, DetachAction&& detachAction)
    {
        using T = std::invoke_result_t<TaskFunc>;

        auto new_task = std::make_unique<QueuedTask<T>>();
        new_task->task = std::forward<TaskFunc>(task);
        new_task->taskFinisher = std::forward<FinisherFunc>(taskFinisher);
        new_task->detachAction = std::forward<DetachAction>(detachAction);

        if(runningThreads < maxThreads) // free thread available
        {
            if(running_tasks.size() == acceptThreads)
            {
                detached_tasks.push_back(std::move(running_tasks.back()));
                running_tasks.pop_back();
            }

            running_tasks.emplace_front(new_task->run(runningThreads));
        }
        else if (queueSize > 0)
        {
            if (queued_tasks.size() == queueSize)
            {
                queued_tasks.pop_front();
            }
            queued_tasks.emplace_back(std::move(new_task));
        }
    }

    void finish();

    void terminateAll();
    void detachAll();
    int get_treads_running();

    virtual ~ParallelTaskManager()
    {
        terminateAll();
    }

    bool idle() const
    {
        return running_tasks.empty()&&detached_tasks.empty();
    }

    bool free() const
    {
        return running_tasks.empty();
    }
};


class AutoParallelTaskManager:public ParallelTaskManager,public vkbase::OnLogicUpdateReceiver
{
public:
    explicit AutoParallelTaskManager(int maxThreads=1,int queue_size=1, bool only_finish_last=false ,int priority=0):
    ParallelTaskManager(maxThreads,queue_size,only_finish_last)
    {
        vkbase::OnLogicUpdateReceiver::disable();
        vkbase::OnLogicUpdateReceiver::setPriority(priority);
    }

    template<typename TaskFunc, typename FinisherFunc>
    void runTask(TaskFunc&& task, FinisherFunc&& taskFinisher)
    {
        ParallelTaskManager::runTask(std::forward<TaskFunc>(task), std::forward<FinisherFunc>(taskFinisher));
        if(! this->running_tasks.empty())
            vkbase::OnLogicUpdateReceiver::enable();
    }

protected:
    void onUpdateLogic(uint32_t imageIndex) override
    {
        this->finish();
        if(this->running_tasks.empty())
            vkbase::OnLogicUpdateReceiver::disable();
    }
};