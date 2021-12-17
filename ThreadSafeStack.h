//
// Created by 王起鹏 on 12/17/21.
//

#ifndef PLAYGROUND_THREADSAFESTACK_H
#define PLAYGROUND_THREADSAFESTACK_H
#include <memory>
#include <mutex>
#include <exception>
#include <queue>

class empty_stack: std::exception {
    const char* what() const throw();
};

template <typename T>
class thread_safe_queue{
    mutable std::mutex mtx_;
    std::queue<T> queue_;

    // waiting by conditional_variable
    std::condition_variable cond_;
public:
    thread_safe_queue() = default;

    thread_safe_queue(const thread_safe_queue& other ){
        std::lock_guard lg(other.mtx_);
        queue_ = other.queue_; // copy everything
    }

    void push(T ele){
        std::lock_guard lg(mtx_);
        queue_.push(std::move(ele));
    }

    std::shared_ptr<T> pop(){
        std::lock_guard lg(mtx_);
        if(queue_.empty()){
            throw empty_stack();
        }
        std::shared_ptr<T> const res(std::make_shared<T>(std::move(queue_.top())));
        queue_.pop();
        return res;
    }
    bool empty() const {
        std::lock_guard lg(mtx_);
       return queue_.empty();
    }

    void produce_thread(){
        while(more_data_to_prepare()){
            Data const data = prepare_data();
            {
                std::lock_guard<std::mutex> lk(mtx_);
                queue_.push(data);
            }
            cond_.notify_one();
        }
    }
    void consume_thread(){
        while(true){
            std::unique_lock<std::mutex> lk(mtx_);
            cond_.wait(lk, [&]{return queue_.empty();});
            Data data= queue_.front();
            queue_.pop();
            lk.unlock();

        }
    }

};


template<typename T>
class threadsafe_queue
{
private:
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;
public:
    threadsafe_queue()
    {}
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(std::move(new_value));
        data_cond.notify_one();                                    1
    }
    void wait_and_pop(T& value)                                    2
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk,[this]{return !data_queue.empty();});
        value=std::move(data_queue.front());
        data_queue.pop();
    }
    std::shared_ptr<T> wait_and_pop()                              3
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk,[this]{return !data_queue.empty();});    4
        std::shared_ptr<T> res(
                std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();
        return res;
    }
    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty())
            return false;
        value=std::move(data_queue.front());
        data_queue.pop();
        return true;
    }
    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty())
            return std::shared_ptr<T>();                           5
        std::shared_ptr<T> res(
                std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();
        return res;
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
};

#endif //PLAYGROUND_THREADSAFESTACK_H
