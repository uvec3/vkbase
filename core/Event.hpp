#pragma once
#include <functional>
#include <algorithm>
#include <map>
namespace vkbase
{
    template<typename... Args>
    struct Callback
    {
        int id;
        int priority;
        std::function<void(Args...)> fun;
    };

    //make parameters for function as template parameters
    template<typename... Args>
    class Event
    {
        std::vector<Callback<Args...>> callbacks;
        int idCounter= 0;
        Callback<Args...>* currentlyExecuting= nullptr;
        int i=0;
    public:
        int addCallback(std::function<void(Args...)> callback, int priority= 0)
        {
            callbacks.push_back(Callback<Args...>{idCounter, priority, callback});
            std::sort(callbacks.begin(), callbacks.end(), [](const Callback<Args...> &a, const Callback<Args...> &b)
            {
                return a.priority > b.priority;
            });
            return idCounter++;
        }
        void removeCallback(int id)
        {
            for (auto it = callbacks.begin(); it != callbacks.end(); ++it)
            {
                if (it->id == id)
                {
                    auto ptr= &(*it);
                    if(ptr<=currentlyExecuting)//if inside callbacks loop, shift i if removing currently executing callback or one before
                        --i;
                    callbacks.erase(it);
                    return;
                }
            }
        }

        void call(Args... args)
        {
            for (i=0;i<callbacks.size(); ++i)
            {
                currentlyExecuting= &callbacks[i];
                callbacks[i].fun(args...);
            }
            currentlyExecuting= nullptr;
        }
    };

    template<typename... Args>
    class AbstractEventReceiver
    {
        int id=-1;
        int priority=0;
    protected:
        Event<Args...>& event;
        std::function<void(Args...)> callback;


        AbstractEventReceiver(Event<Args...>& event, std::function<void(Args...)> callback):
        event{event}, callback{callback}
        {
            enable();
        }

    public:
        void enable(bool flag)
        {
            if(flag)
                enable();
            else
                disable();
        }

        void enable()
        {
            if(id==-1)
                id=event.addCallback(callback, priority);
        }

        void disable()
        {
            if(id!=-1)
            {
                event.removeCallback(id);
                id=-1;
            }
        }

        void setPriority(int newPriority)
        {
            if(id!=-1 && priority!=newPriority)
            {
                priority=newPriority;
                disable();
                enable();
            }
            priority=newPriority;
        }

        virtual ~AbstractEventReceiver()
        {
            disable();
        }
    };
}