//
// Created by 王起鹏 on 12/17/21.
//

#ifndef PLAYGROUND_SPSCQUEUE_H
#define PLAYGROUND_SPSCQUEUE_H
#include <atomic>
#include <array>

// ring buffer
template<typename T, size_t SIZE = 10>
class spsc{
    std::atomic<int> size_;
    std::array<T, SIZE> queue;

public:
    void produce(T ele){

    }

    T consume(){

    }
};


#endif //PLAYGROUND_SPSCQUEUE_H
