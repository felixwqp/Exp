//
// Created by 王起鹏 on 12/17/21.
//

#ifndef PLAYGROUND_THREADPOOL_H
#define PLAYGROUND_THREADPOOL_H

class thread_pool
{
    std::atomic_bool done;
    threadsafe_queue<std::function<void()> > work_queue;                   1
    std::vector<std::thread> threads;                                      2
    join_threads joiner;                                                   3
    void worker_thread()
    {
        while(!done)                                                       4
        {
            std::function<void()> task;
            if(work_queue.try_pop(task))                                   5
            {
                task();                                                    6
            }
            else
            {
                std::this_thread::yield();                                 7
            }
        }
    }
public:
    thread_pool():
            done(false),joiner(threads)
    {
        unsigned const thread_count=std::thread::hardware_concurrency();   8
        try
        {
            for(unsigned i=0;i<thread_count;++i)
            {
                threads.push_back(
                        std::thread(&thread_pool::worker_thread,this));        9
            }
        }
        catch(...)
        {
            done=true;                                                     10
            throw;
        }
    }
    ~thread_pool()
    {
        done=true;                                                         11
    }
    template<typename FunctionType>
    void submit(FunctionType f)
    {
        work_queue.push(std::function<void()>(f));                         12
    }
};


class thread_pool
{
    threadsafe_queue<function_wrapper> pool_work_queue;
    typedef std::queue<function_wrapper> local_queue_type;   1
    static thread_local std::unique_ptr<local_queue_type>
            local_work_queue;                                    2
    void worker_thread()
    {
        local_work_queue.reset(new local_queue_type);        3

        while(!done)
        {
            run_pending_task();
        }
    }
public:
    template<typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type>
    submit(FunctionType f)
    {
        typedef typename std::result_of<FunctionType()>::type result_type;
        std::packaged_task<result_type()> task(f);
        std::future<result_type> res(task.get_future());
        if(local_work_queue)                                 4
        {
            local_work_queue->push(std::move(task));
        }
        else
        {
            pool_work_queue.push(std::move(task));           5
        }
        return res;
    }
    void run_pending_task()
    {
        function_wrapper task;
        if(local_work_queue && !local_work_queue->empty())  6
        {
            task=std::move(local_work_queue->front());
            local_work_queue->pop();
            task();
        }
        else if(pool_work_queue.try_pop(task))              7
        {
            task();
        }
        else
        {
            std::this_thread::yield();
        }
    }
    // rest as before
};
#endif //PLAYGROUND_THREADPOOL_H
