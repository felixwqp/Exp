//
// Created by 王起鹏 on 12/17/21.
//

#ifndef PLAYGROUND_SPINLOCK_H
#define PLAYGROUND_SPINLOCK_H

class spinlock_mutex
{
    std::atomic_flag flag;
public:
    spinlock_mutex():
            flag(ATOMIC_FLAG_INIT)
    {}
    void lock()
    {
        while(flag.test_and_set(std::memory_order_acquire));
    }
    void unlock()
    {
        flag.clear(std::memory_order_release);
    }
};

#endif //PLAYGROUND_SPINLOCK_H
