#include <functional>
#include <thread>
#include <condition_variable>
#include <shared_mutex>
#include <atomic>

#include "../Leetcode/leetcode.h"


struct Task{
    long execute_time;
    function<void()> task;
};

struct TaskCompare{
    bool operator() (Task a, Task b){
        return a.execute_time < b.execute_time;        
    }
};

struct PriorityQueue
{

private:
    // using ReadLock = std::shared_lock<std::shared_mutex>;
    using WriteLock = std::unique_lock<std::shared_mutex>;
    priority_queue<Task, vector<Task>, TaskCompare> task_queue;
    std::shared_mutex queue_mtx;

    static long get_cur_time(){
        auto now = std::chrono::system_clock::now();
        auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now); 
        auto epoch = now_ms.time_since_epoch();
        auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch); 

       return value.count();
   } 

private:    

  // pop an element and return a copy. Block if queue empty.
  bool try_pop(function<void()>& ele){
      WriteLock w_lock(queue_mtx);
      int cur_time = get_cur_time();
    //   cout << " try to get" << endl;
        cout <<"CUR: " <<  cur_time << endl;
      if(task_queue.empty() || task_queue.top().execute_time > cur_time){
          return false;
      }
      cout << "Got Item" << std::endl;
      auto& top_task = task_queue.top();
      task_queue.pop();
      ele = std::move(top_task.task);
      return true;

  }
  //  push an element to the back of the queue.
  void push(const Task& item){
      WriteLock w_lock(queue_mtx);
      cout << "Push Item" << std::endl;
      task_queue.push(item);
  }
private:
    std::atomic_bool done;
    std::vector<std::thread> threads;
    void worker_thread(){
        while(!done){
            std::function<void()> task;
            if(try_pop(task)){
                task();
            }else{
                std::this_thread::yield();
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
       long cur_time = get_cur_time();
       long execute_time = cur_time + second_count * 1000; 
       cout << execute_time << std::endl;
       push(Task{execute_time, task});
   }
};


int main(){
    PriorityQueue q;
    /*
    std::thread t1([&](){
        cout << "add task at t=0" << endl;
        auto print_sth = [](){
            cout << "Execute task at t = 2" << std::endl;
        };
        q.submit_task(std::move(print_sth), 2);
    });
    */
        auto print_sth = [](){
            cout << "Execute task at t = 2" << std::endl;
        };
        q.submit_task(std::move(print_sth), 2);
}