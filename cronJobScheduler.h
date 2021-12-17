//
// Created by 王起鹏 on 12/14/21.
//

#ifndef PLAYGROUND_CRONJOBSCHEDULER_H
#define PLAYGROUND_CRONJOBSCHEDULER_H
#include <iostream>
#include <mutex>
#include <list>
#include <memory>
#include <map>
#include <functional>
#include <vector>


/*
 * multimap: move log(N)
 * priority_queue -> better cache locality, actually it's heap
 *
 * std::function: on heap -> dynamic memory
 *
 *      1.  When calling a std::function, it does a virtual function call.
        2. When assigning a lambda with significant captures to a std::function, it will do a dynamic memory allocation!

1. move/copy/temp obj ->
    calling stack frame,
    copy into add_evet param,
    copy to temp struct,
    store in struct


    std::chrono
 * */



/*
 *  _STRUCT_TIMEVAL
    {
	    __darwin_time_t         tv_sec;
        __darwin_suseconds_t    tv_usec;
    };
 *
 * */
struct Job {
    Job() = default;
    template <typename _Callable, typename... _Args>
    explicit Job(bool reiterable, int interval,
                 _Callable&& func, _Args&& ... args)
                 : reiterable_(reiterable)
                 , interval_(interval)
                 , function_(std::bind(func, args))
                 {
        if(interval_ < 1){
            throw std::runtime_error("Interval shall large than 1");
        }
    }

//private:
    // friend class Scheduler;

    bool reiterable_ = false;
    int interval_ = 1;
    std::function<void()> function_;
};





struct Scheduler{
public:
    Scheduler() = default;
    void OnNewTime(const timeval& time){
        std::lock_guard<std::mutex> guard(lock_);
        // if time  == cur: nothing will happen
        if(time.tv_sec == current_time_) return;
        // if time < cur: 1. no job run; 2. jobs reschedule
        // if time > cur: 1. run jobs, 2. reschedule reiterable one
        auto old_time = current_time_;
        current_time_ = time.tv_sec;

        if(job_map_.empty()) return;

        std::vector<std::shared_ptr<Job>> new_jobs{};
        if(old_time > current_time_){
            // Reschedule
            Reschedule();
        }else{
            auto cur_ptr = job_map_.upper_bound(current_time_);
            for(auto it = job_map_.begin(); it != cur_ptr; it++){
                new_jobs.push_back(std::move(it->second));
                if(it->second->reiterable_){
                    job_map_.emplace(it->second->interval_ + current_time_, new_jobs.back());
                }
            }
            job_map_.erase(job_map_.begin(), cur_ptr);
        }
        for(const auto& job: new_jobs){
            job -> function_();
        }
    }

    template <typename _Callable, typename... _Args>
    std::shared_ptr<Job> Run(int interval, _Callable&& func, _Args&&... args){
        // add new repetitive job to scheduler
        return Add(true, interval, std::forward<_Callable>(func), std::forward<_Args>(args)...);
    }

    template<typename _Callable, typename... _Args>
    std::shared_ptr<Job> RunOnce(int interval, _Callable&& func, _Args&&... args){
        return Add(false, interval, std::forward<_Callable>(func), std::forward<_Args>(args)...);
    }

    void Remove(std::shared_ptr<Job>& job){

    }



private:
    template<typename _Callable, typename... _Args>
    std::shared_ptr<Job> Add(_Callable&& func, _Args&&... args, bool is_iterate, int interval){
        std::lock_guard<std::mutex> guard(lock_);
        auto it = job_map_.emplace(
                is_iterate,
                current_time_ + interval,
                std::forward<_Callable>(func),
                std::forward<_Args>(args)...
                );
        return it -> second;
    }

    void Reschedule(){
        std::multimap<long, std::shared_ptr<Job>> new_job_map_{};
        for(auto job: job_map_){
            new_job_map_.emplace(
                    current_time_ + job.first,
                    std::move(job.second)
                    );
        }
        job_map_ = std::move(new_job_map_);
    }



private:
    std::mutex lock_;
    long current_time_ = 0;
    std::multimap<long, std::shared_ptr<Job>> job_map_;
};


int main(){

}

#endif //PLAYGROUND_CRONJOBSCHEDULER_H
