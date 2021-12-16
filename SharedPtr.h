//
// Created by 王起鹏 on 12/15/21.
//

#ifndef PLAYGROUND_SHAREDPTR_H
#define PLAYGROUND_SHAREDPTR_H
#include <mutex>
#include <memory>
#include <iostream>
#include <thread>

template<typename T>
class SimplePtr {
private:
    int* ref_cnt_;
    T* val_;
    std::mutex* mtx_;

public:
    SimplePtr()
        : ref_cnt_(new int(1))
        , val_(T{})
        , mtx_(new std::mutex){}

    SimplePtr(T data)
            : ref_cnt_(new int(1))
            , val_(data)
            , mtx_(new std::mutex){}

    SimplePtr(const SimplePtr<T>&  sp)
            : ref_cnt_(sp.ref_cnt_)
            , val_(sp.val_)
            , mtx_(sp.mtx_)
    {
        std::scoped_lock lk(*mtx_);
        ++(*ref_cnt_);
    }

    SimplePtr<T>& operator= (const SimplePtr<T>& ptr){
        if(this == & ptr) return this;
        release(); // release current resource

        mtx_ = ptr.mtx_;
        ref_cnt_ = ptr.ref_cnt_;
        val_ = ptr.val_;
        add_ref_count();
        return *this;
    }

private:

    void add_ref_count(){
        std::lock_guard<std::mutex> lg(*mtx_);
        ++(*ref_cnt_);
    }
    void release(){
        bool flag = false;
        {
            std::lock_guard<std::mutex> lk(*mtx_);
            if( --(*ref_cnt_) == 0 ){
                delete ref_cnt_;
                delete val_;
                flag = true;
            }
        }
        if(flag){
            delete mtx_;
        }
    }



};


#endif //PLAYGROUND_SHAREDPTR_H
