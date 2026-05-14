#include "ParallelTaskManager.hpp"

#include "EngineBase.h"

void ParallelTaskManager::finish()
{
    while(!running_tasks.empty()&&running_tasks.back()->finished.load())
    {
        running_tasks.back()->thread.join();
        running_tasks.back()->callFinisher();
        running_tasks.pop_back();

        if (!queued_tasks.empty())
        {
            running_tasks.push_back(queued_tasks.front()->run(runningThreads));
            queued_tasks.pop_front();
        }


        auto it = detached_tasks.begin();
        for (; it != detached_tasks.end();)
        {
            if ((*it)->finished.load())
            {
                (*it)->thread.join();
                (*it)->callDetachAction();
                it = detached_tasks.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}

void ParallelTaskManager::terminateAll()
{
    for(auto& task:running_tasks)
    {
        //task->thread.request_stop();
        task->thread.join();
        task->callDetachAction();
    }
    running_tasks.clear();

    for (auto& task:detached_tasks)
    {
        //task->thread.request_stop();
        task->thread.join();
        task->callDetachAction();
    }
    detached_tasks.clear();
}

void ParallelTaskManager::detachAll()
{
    if (!idle())
    {
        vkbase::runTask([running_tasks=std::move(running_tasks),detached_tasks=std::move(detached_tasks)]()
        {
            for(auto& task:running_tasks)
            {
                task->thread.request_stop();
                task->thread.join();
                task->callDetachAction();
            }

            for (auto& task:detached_tasks)
            {
                task->thread.request_stop();
                task->thread.join();
                task->callDetachAction();
            }

            return true;
        },[](const auto&){});
    }
}

int ParallelTaskManager::get_treads_running()
{
    return runningThreads;
}
