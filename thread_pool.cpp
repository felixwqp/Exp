#include <vector>
#include <queue>
#include <cassert>
#include <cstring>
#include <stdint.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>
#include <cassert>
#include <unistd.h>
#include <thread>





typedef void (*work_fn)();
class ThreadPool {
    
public:
    ThreadPool() : _stop(true), _thread_count(0) {}
    ThreadPool(uint32_t t) : _stop(true), _thread_count(t) {}
    ~ThreadPool();
    
    void start();
    void stop();
    void add_work(work_fn task);
    
private:
    void thread_entry();
    
    std::vector<std::thread> _thread_list;
    std::queue<work_fn> _task_list;
    std::mutex _lock;
    std::condition_variable _mon;
    bool _stop;
    uint32_t _thread_count;
    
};


ThreadPool::~ThreadPool()
{
    stop();
    for (uint32_t i = 0; i < _thread_count; ++i) {
	_thread_list[i].join();
    }
}


void ThreadPool::start()
{
    _lock.lock();
    _stop = false;
    
    if (!_thread_count) {
	_thread_count = std::thread::hardware_concurrency();
	assert(_thread_count > 0);
    }
    
    for (uint32_t i = 0; i < _thread_count; ++i) {
	_thread_list.push_back(std::thread(&ThreadPool::thread_entry, this));
    }
    
    _lock.unlock();
}


void ThreadPool::stop()
{
    _lock.lock();
    
    _stop = true;
    _mon.notify_all();
    
    _lock.unlock();
}


void ThreadPool::add_work(work_fn task)
{
    _lock.lock();
    
    _task_list.push(task);
    _mon.notify_one();
    
    _lock.unlock();
}


void ThreadPool::thread_entry()
{
    work_fn work;
    
    while (true) {
        _lock.lock();
        std::unique_lock<std::mutex> u_lck(_lock, std::adopt_lock);
        
        while(_task_list.empty()) {
            if (_stop) {
                u_lck.unlock();
                return;
            }
            
            _mon.wait(u_lck);
        }
        
        work = _task_list.front();
        _task_list.pop();
        
        u_lck.unlock();
        work();
    }
}



void work() {
    for(int i = 0; i < 100; ++i) {
	if (i == 56) {
	    std::cout << "count is 56!\n\n";
	    std::cout.flush();
	}
    }
    sleep(1);
}

int main() {
    
    ThreadPool pool;
    
    pool.add_work(&work);
    pool.start();
    
    sleep(3);
    pool.stop();
    return(0);
}