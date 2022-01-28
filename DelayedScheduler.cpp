// https://stackoverflow.com/questions/46094951/how-to-create-an-efficient-multi-threaded-task-scheduler-in-c/46149712#46149712?newreg=e375ee1902da489eb0135d1b9db5b23c
// https://codereview.stackexchange.com/questions/21336/c-task-scheduler

#include <functional>
#include <thread>
#include <condition_variable>
#include <shared_mutex>
#include <atomic>
#include <memory>
#include "../Leetcode/leetcode.h"


struct Task{
    std::chrono::system_clock::time_point execute_time;
    mutable bool is_assign;
    function<void()> task;
    Task(std::chrono::system_clock::time_point time_, bool assign_, function<void()>&& task_)
        : execute_time(time_)
        , is_assign(assign_)
        , task((task_)) {}
};

struct TaskCompare{
    bool operator() (Task a, Task b){
        return a.execute_time >
         b.execute_time;        
    }
};

struct PriorityQueue
{

private:
    // using ReadLock = std::shared_lock<std::shared_mutex>;
    using WriteLock = std::unique_lock<std::shared_mutex>;
    priority_queue<Task, vector<Task>, TaskCompare> task_queue;
    std::mutex queue_mtx;
    std::condition_variable queue_cv;
    std::condition_variable task_cv;


private:
    std::atomic_bool done;
    std::vector<std::thread> threads;
    void worker_thread(){
        std::unique_lock<std::mutex> lock(queue_mtx);
        while(!done){
            if(!task_queue.empty() && task_queue.top().execute_time < std::chrono::system_clock::now()){
                std::function<void()> task = std::move(task_queue.top().task);
                bool is_assign = task_queue.top().is_assign;
                if(is_assign){
                    task_cv.notify_all();
                }else{
                    queue_cv.notify_all();
                }
                task_queue.pop();
                lock.unlock();
                task();
                lock.lock();
            }else if(!task_queue.empty() && task_queue.top().is_assign == false){
                task_queue.top().is_assign = true;
                task_cv.wait_until(lock, task_queue.top().execute_time);
                task_queue.top().is_assign = false;
            }else{
                queue_cv.wait(lock);                
            }
        }
    }


public:
    PriorityQueue() :done(false){
        // unsigned const thread_count = std::thread::hardware_concurrency();
        unsigned const thread_count = 2;
        try {
            for(int i = 0; i < thread_count; ++i){
                threads.push_back(std::thread(&PriorityQueue::worker_thread, this));             
            }
        }catch(...){
            done = true;
            throw;
        }

    }
    ~PriorityQueue(){
        
        for(auto& thread: threads){
            if(thread.joinable()){
                thread.join();
            }
        }
        done = true;
    }

public:
    // assume success

   void submit_task(function<void()> task, int second_count){
       
       std::chrono::system_clock::time_point execute_time = std::chrono::system_clock::now() 
        +  std::chrono::seconds(second_count); 
       std::unique_lock<std::mutex> lock(queue_mtx);
       task_queue.push(Task(execute_time, false, std::move(task)));
       queue_cv.notify_all();
   }
};



int main(){
    PriorityQueue q;

    cout << "start to execute" << endl;
    auto print_sth_2s = [](){
        cout << "Execute task at t = 2" << std::endl;
    };
    q.submit_task(std::move(print_sth_2s), 2);
    auto print_sth_5s = [](){
        cout << "Execute task at t = 5" << std::endl;
    };
    q.submit_task(std::move(print_sth_5s), 5);
    auto print_sth_8s = [](){
        cout << "Execute task at t = 8" << std::endl;
    };
    q.submit_task(std::move(print_sth_8s), 8);
}