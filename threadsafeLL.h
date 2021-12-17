//
// Created by 王起鹏 on 12/17/21.
//

#ifdef Temp
#include <memory>
#include <mutex>

template<typename T>
class queue
{
private:
    struct node
    {
        T data;
        std::unique_ptr<node> next;
        node(T data_):
                data(std::move(data_))
        {}
    };
    std::unique_ptr<node> head;                     1
    node* tail;                                     2
public:
    queue(): tail(nullptr)
    {}
    queue(const queue& other)=delete;
    queue& operator=(const queue& other)=delete;
    std::shared_ptr<T> try_pop()
    {
        if(!head)
        {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> const res(
                std::make_shared<T>(std::move(head->data)));
        std::unique_ptr<node> const old_head=std::move(head);
        head=std::move(old_head->next);            3
        if(!head)
            tail=nullptr;
        return res;
    }
    void push(T new_value)
    {
        std::unique_ptr<node> p(new node(std::move(new_value)));
        node* const new_tail=p.get();
        if(tail)
        {
            tail->next=std::move(p);               4
        }
        else
        {
            head=std::move(p);                     5
        }
        tail=new_tail;                             6
    }
};


template<typename T>
class queue
{
private:
    struct node
    {
        std::shared_ptr<T> data;                        1
        std::unique_ptr<node> next;
    };
    std::unique_ptr<node> head;
    node* tail;
public:
    queue():
            head(new node),tail(head.get())                 2
    {}
    queue(const queue& other)=delete;
    queue& operator=(const queue& other)=delete;
    std::shared_ptr<T> try_pop()
    {
        if(head.get()==tail)                            3
        {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> const res(head->data);       4
        std::unique_ptr<node> old_head=std::move(head);
        head=std::move(old_head->next);                 5
        return res;                                     6
    }
    void push(T new_value)
    {
        std::shared_ptr<T> new_data(
                std::make_shared<T>(std::move(new_value))); 7
        std::unique_ptr<node> p(new node);              8
        tail->data=new_data;                            9
        node* const new_tail=p.get();
        tail->next=std::move(p);
        tail=new_tail;
    }
};

#endif

#define Final

#ifdef Final

template<typename T>
class threadsafe_queue
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    std::mutex head_mutex;
    std::unique_ptr<node> head;
    std::mutex tail_mutex;
    node* tail;
    std::condition_variable data_cond;

private:
    node* get_tail()
    {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }
    std::unique_ptr<node> pop_head()                             1
    {

    }
    std::unique_lock<std::mutex> wait_for_data()                 2
    {
        std::unique_lock<std::mutex> head_lock(head_mutex);
        data_cond.wait(head_lock,[&]{return head.get()!=get_tail();});
        return std::move(head_lock);                             3
    }
    std::unique_ptr<node> wait_pop_head()
    {
        std::unique_lock<std::mutex> head_lock(head_mutex);
        data_cond.wait(head_lock,[&]{return head.get()!=get_tail();});

        std::unique_ptr<node> old_head=std::move(head);
        head=std::move(old_head->next);
        return old_head;
    }
    /*
    std::unique_ptr<node> wait_pop_head(T& value)
    {
        std::unique_lock<std::mutex> head_lock(wait_for_data()); 5
        value=std::move(*head->data);
        return pop_head();
    }
     */
public:

    /*
    void wait_and_pop(T& value)
    {
        std::unique_ptr<node> const old_head=wait_pop_head(value);
    }
     */
public:
    threadsafe_queue():
        head(new node),tail(head.get())
    {}
    threadsafe_queue(const threadsafe_queue& other)=delete;
    threadsafe_queue& operator=(const threadsafe_queue& other)=delete;
    std::shared_ptr<T> try_pop();
    bool try_pop(T& value);
    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_ptr<node> const old_head=wait_pop_head();
        return old_head->data;
    }
    // void wait_and_pop(T& value);
    void push(T new_value){
        std::shared_ptr<T> new_data(
                std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);
        {
            std::lock_guard<std::mutex> tail_lock(tail_mutex);
            tail->data=new_data;
            node* const new_tail=p.get();
            tail->next=std::move(p);
            tail=new_tail;
        }
        data_cond.notify_one();
    }
    bool empty();
};


Chapter 6. Designing lock-based concurrent data structures

This chapter covers

        What it means to design data structures for concurrency
        Guidelines for doing so
Example implementations of data structures designed for concurrency
        In the last chapter we looked at the low-level details of atomic operations and the memory model. In this chapter we’ll take a break from the low-level details (although we’ll need them for chapter 7) and think about data structures.

The choice of data structure to use for a programming problem can be a key part of the overall solution, and parallel programming problems are no exception. If a data structure is to be accessed from multiple threads, either it must be completely immutable so the data never changes and no synchronization is necessary, or the program must be designed to ensure that changes are correctly synchronized between threads. One option is to use a separate mutex and external locking to protect the data, using the techniques we looked at in chapters 3 and 4, and another is to design the data structure itself for concurrent access.

When designing a data structure for concurrency, you can use the basic building blocks of multithreaded applications from earlier chapters, such as mutexes and condition variables. Indeed, you’ve already seen a couple of examples showing how to combine these building blocks to write data structures that are safe for concurrent access from multiple threads.

In this chapter we’ll start by looking at some general guidelines for designing data structures for concurrency. We’ll then take the basic building blocks of locks and condition variables and revisit the design of those basic data structures before moving on to more complex data structures. In chapter 7 we’ll look at how to go right back to basics and use the atomic operations described in chapter 5 to build data structures without locks.

So, without further ado, let’s look at what’s involved in designing a data structure for concurrency.

6.1. What does it mean to design for concurrency?

At the basic level, designing a data structure for concurrency means that multiple threads can access the data structure concurrently, either performing the same or distinct operations, and each thread will see a self-consistent view of the data structure. No data will be lost or corrupted, all invariants will be upheld, and there’ll be no problematic race conditions. This data structure is said to be thread-safe. In general, a data structure will be safe only for particular types of concurrent access. It may be possible to have multiple threads performing one type of operation on the data structure concurrently, whereas another operation requires exclusive access by a single thread. Alternatively, it may be safe for multiple threads to access a data structure concurrently if they’re performing different actions, whereas multiple threads performing the same action would be problematic.

Truly designing for concurrency means more than that, though: it means providing the opportunity for concurrency to threads accessing the data structure. By its nature, a mutex provides mutual exclusion: only one thread can acquire a lock on the mutex at a time. A mutex protects a data structure by explicitly preventing true concurrent access to the data it protects.

This is called serialization: threads take turns accessing the data protected by the mutex; they must access it serially rather than concurrently. Consequently, you must put careful thought into the design of the data structure to enable true concurrent access. Some data structures have more scope for true concurrency than others, but in all cases the idea is the same: the smaller the protected region, the fewer operations are serialized, and the greater the potential for concurrency.

Before we look at some data structure designs, let’s have a quick look at some simple guidelines for what to consider when designing for concurrency.

6.1.1. Guidelines for designing data structures for concurrency

        As I mentioned, you have two aspects to consider when designing data structures for concurrent access: ensuring that the accesses are safe and enabling genuine concurrent access. I covered the basics of how to make the data structure thread-safe back in chapter 3:

Ensure that no thread can see a state where the invariants of the data structure have been broken by the actions of another thread.
Take care to avoid race conditions inherent in the interface to the data structure by providing functions for complete operations rather than for operation steps.
Pay attention to how the data structure behaves in the presence of exceptions to ensure that the invariants are not broken.
Minimize the opportunities for deadlock when using the data structure by restricting the scope of locks and avoiding nested locks where possible.
Before you think about any of these details, it’s also important to think about what constraints you want to put on the users of the data structure; if one thread is accessing the data structure through a particular function, which functions are safe to call from other threads?

This is a crucial question to consider. Generally, constructors and destructors require exclusive access to the data structure, but it’s up to the user to ensure that they’re not accessed before construction is complete or after destruction has started. If the data structure supports assignment, swap(), or copy construction, then as the designer of the data structure, you need to decide whether these operations are safe to call concurrently with other operations or whether they require the user to ensure exclusive access even though the majority of functions for manipulating the data structure may be called from multiple threads concurrently without any problems.

The second aspect to consider is that of enabling genuine concurrent access. I can’t offer much in the way of guidelines for this; instead, here’s a list of questions to ask yourself as the data structure designer:

Can the scope of locks be restricted to allow some parts of an operation to be performed outside the lock?
Can different parts of the data structure be protected with different mutexes?
Do all operations require the same level of protection?
Can a simple change to the data structure improve the opportunities for concurrency without affecting the operational semantics?
All these questions are guided by a single idea: how can you minimize the amount of serialization that must occur and enable the greatest amount of true concurrency? It’s not uncommon for data structures to allow concurrent access from multiple threads that merely read the data structure, whereas a thread that can modify the data structure must have exclusive access. This is supported by using constructs like std::shared_mutex. Likewise, as you’ll see shortly, it’s quite common for a data structure to support concurrent access from threads performing different operations while serializing threads that try to perform the same operation.

The simplest thread-safe data structures typically use mutexes and locks to protect the data. Although there are issues with this, as you saw in chapter 3, it’s relatively easy to ensure that only one thread is accessing the data structure at a time. To ease you into the design of thread-safe data structures, we’ll stick to looking at such lock-based data structures in this chapter and leave the design of concurrent data structures without locks for chapter 7.

6.2. Lock-based concurrent data structures

The design of lock-based concurrent data structures is all about ensuring that the right mutex is locked when accessing the data and that the lock is held for the minimum amount of time. This is hard enough when there’s just one mutex protecting a data structure. You need to ensure that data can’t be accessed outside the protection of the mutex lock and that there are no race conditions inherent in the interface, as you saw in chapter 3. If you use separate mutexes to protect separate parts of the data structure, these issues are compounded, and there’s now also the possibility of deadlock if the operations on the data structure require more than one mutex to be locked. You therefore need to consider the design of a data structure with multiple mutexes even more carefully than the design of a data structure with a single mutex.

In this section you’ll apply the guidelines from section 6.1.1 to the design of several simple data structures, using mutexes and locks to protect the data. In each case you’ll seek out opportunities for enabling greater concurrency while ensuring that the data structure remains thread-safe.

Let’s start by looking at the stack implementation from chapter 3; it’s one of the simplest data structures around, and it uses only a single mutex. Is it thread-safe? How does it fare from the point of view of achieving true concurrency?

6.2.1. A thread-safe stack using locks

The thread-safe stack from chapter 3 is reproduced in the following listing. The intent is to write a thread-safe data structure akin to std::stack<>, which supports pushing data items onto the stack and popping them off again.

Listing 6.1. A class definition for a thread-safe stack

#include <exception>
struct empty_stack: std::exception
{
    const char* what() const throw();
};
template<typename T>
class threadsafe_stack
{
private:
    std::stack<T> data;
    mutable std::mutex m;
public:
    threadsafe_stack(){}
    threadsafe_stack(const threadsafe_stack& other)
    {
        std::lock_guard<std::mutex> lock(other.m);
        data=other.data;
    }
    threadsafe_stack& operator=(const threadsafe_stack&) = delete;
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lock(m);
        data.push(std::move(new_value));                  1
    }
    std::shared_ptr<T> pop()
    {
        std::lock_guard<std::mutex> lock(m);
        if(data.empty()) throw empty_stack();             2
        std::shared_ptr<T> const res(
                std::make_shared<T>(std::move(data.top())));   3
        data.pop();                                       4
        return res;
    }
    void pop(T& value)
    {
        std::lock_guard<std::mutex> lock(m);
        if(data.empty()) throw empty_stack();
        value=std::move(data.top());                      5
        data.pop();                                       6
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};
Let’s look at each of the guidelines in turn and see how they apply here.

First, as you can see, the basic thread safety is provided by protecting each member function with a lock on the mutex, m. This ensures that only one thread is accessing the data at any one time, so provided each member function maintains the invariants, no thread can see a broken invariant.

Second, there’s a potential for a race condition between empty() and either of the pop() functions, but because the code explicitly checks for the contained stack being empty while holding the lock in pop(), this race condition isn’t problematic. By returning the popped data item directly as part of the call to pop(), you avoid a potential race condition that would be present with separate top() and pop() member functions such as those in std::stack<>.

Next, there are a few potential sources of exceptions. Locking a mutex may throw an exception, but not only is this likely to be exceedingly rare (because it indicates a problem with the mutex or a lack of system resources), it’s also the first operation in each member function. Because no data has been modified, this is safe. Unlocking a mutex can’t fail, so that’s always safe, and the use of std::lock_guard<> ensures that the mutex is never left locked.

The call to data.push() 1 may throw an exception if either copying/moving the data value throws an exception or not enough memory can be allocated to extend the underlying data structure. Either way, std::stack<> guarantees it will be safe, so that’s not a problem either.

In the first overload of pop(), the code itself might throw an empty_stack exception 2, but nothing has been modified, so that’s safe. The creation of res 3 might throw an exception, though, for a couple of reasons: the call to std::make_shared might throw because it can’t allocate memory for the new object and the internal data required for reference counting, or the copy constructor or move constructor of the data item to be returned might throw when copying/moving into the freshly-allocated memory. In both cases, the C++ runtime and Standard Library ensure that there are no memory leaks and the new object (if any) is correctly destroyed. Because you still haven’t modified the underlying stack, you’re OK. The call to data.pop() 4 is guaranteed not to throw, as is the return of the result, so this overload of pop() is exception-safe.

The second overload of pop() is similar, except this time it’s the copy assignment or move assignment operator that can throw 5, rather than the construction of a new object and an std::shared_ptr instance. Again, you don’t modify the data structure until the call to data.pop() 6, which is still guaranteed not to throw, so this overload is exception-safe too.

Finally, empty() doesn’t modify any data, so that’s exception-safe.

There are a couple of opportunities for deadlock here, because you call user code while holding a lock: the copy constructor or move constructor (1, 3) and copy assignment or move assignment operator 5 on the contained data items, as well as potentially a user-defined operator new. If these functions either call member functions on the stack that the item is being inserted into or removed from or require a lock of any kind and another lock was held when the stack member function was invoked, there’s the possibility of deadlock. But it’s sensible to require that users of the stack be responsible for ensuring this; you can’t reasonably expect to add an item onto a stack or remove it from a stack without copying it or allocating memory for it.

Because all the member functions use std::lock_guard<> to protect the data, it’s safe for any number of threads to call the stack member functions. The only member functions that aren’t safe are the constructors and destructors, but this isn’t a problem; the object can be constructed only once and destroyed only once. Calling member functions on an incompletely constructed object or a partially destructed object is never a good idea, whether done concurrently or not. As a consequence, the user must ensure that other threads aren’t able to access the stack until it’s fully constructed and must ensure that all threads have ceased accessing the stack before it’s destroyed.

Although it’s safe for multiple threads to call the member functions concurrently, because of the use of locks, only one thread is ever doing any work in the stack data structure at a time. This serialization of threads can potentially limit the performance of an application where there’s significant contention on the stack: while a thread is waiting for the lock, it isn’t doing any useful work. Also, the stack doesn’t provide any means of waiting for an item to be added, so if a thread needs to wait, it must periodically call empty(), or call pop() and catch the empty_stack exceptions. This makes this stack implementation a poor choice if such a scenario is required, because a waiting thread must either consume precious resources checking for data or the user must write external wait and notification code (for example, using condition variables), which might render the internal locking unnecessary and therefore wasteful. The queue from chapter 4 shows a way of incorporating this waiting into the data structure itself using a condition variable inside the data structure, so let’s look at that next.

6.2.2. A thread-safe queue using locks and condition variables

The thread-safe queue from chapter 4 is reproduced in listing 6.2. Much like the stack was modeled after std::stack<>, this queue is modeled after std::queue<>. Again, the interface differs from that of the standard container adaptor because of the constraints of writing a data structure that’s safe for concurrent access from multiple threads.

Listing 6.2. The full class definition for a thread-safe queue using condition variables

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
The structure of the queue implementation shown in listing 6.2 is similar to the stack from listing 6.1, except for the call to data_cond.notify_one() in push() 1 and the wait_and_pop() functions, 2 and 3. The two overloads of try_pop() are almost identical to the pop() functions from listing 6.1, except that they don’t throw an exception if the queue is empty. Instead, they return either a bool value indicating whether a value was retrieved or a NULL pointer if no value could be retrieved by the pointer-returning overload 5. This would also have been a valid way of implementing the stack. If you exclude the wait_and_pop() functions, the analysis you did for the stack applies just as well here.

The new wait_and_pop() functions are a solution to the problem of waiting for a queue entry that you saw with the stack; rather than continuously calling empty(), the waiting thread can call wait_and_pop() and the data structure will handle the waiting with a condition variable. The call to data_cond.wait() won’t return until the underlying queue has at least one element, so you don’t have to worry about the possibility of an empty queue at this point in the code, and the data is still protected with the lock on the mutex. These functions don’t therefore add any new race conditions or possibilities for deadlock, and the invariants will be upheld.

There’s a slight twist with regard to exception safety in that if more than one thread is waiting when an entry is pushed onto the queue, only one thread will be woken by the call to data_cond.notify_one(). But if that thread then throws an exception in wait_and_pop(), such as when the new std::shared_ptr<> is constructed 4, none of the other threads will be woken. If this isn’t acceptable, the call is readily replaced with data_cond.notify_all(), which will wake all the threads but at the cost of most of them then going back to sleep when they find that the queue is empty after all. A second alternative is to have wait_and_pop() call notify_one() if an exception is thrown, so that another thread can attempt to retrieve the stored value. A third alternative is to move the std::shared_ptr<> initialization to the push() call and store std::shared_ptr<> instances rather than direct data values. Copying the std::shared_ptr<> out of the internal std::queue<> then can’t throw an exception, so wait_and_pop() is safe again. The following listing shows the queue implementation revised with this in mind.

Listing 6.3. A thread-safe queue holding std::shared_ptr<> instances

template<typename T>
class threadsafe_queue
{
private:
    mutable std::mutex mut;
    std::queue<std::shared_ptr<T> > data_queue;
    std::condition_variable data_cond;
public:
    threadsafe_queue()
    {}
    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk,[this]{return !data_queue.empty();});
        value=std::move(*data_queue.front());                   1
        data_queue.pop();
    }
    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty())
            return false;
        value=std::move(*data_queue.front());                   2
        data_queue.pop();
        return true;
    }
    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk,[this]{return !data_queue.empty();});
        std::shared_ptr<T> res=data_queue.front();              3
        data_queue.pop();
        return res;
    }
    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res=data_queue.front();              4
        data_queue.pop();
        return res;
    }
    void push(T new_value)
    {
        std::shared_ptr<T> data(
                std::make_shared<T>(std::move(new_value)));         5
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(data);
        data_cond.notify_one();
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
};
The basic consequences of holding the data by std::shared_ptr<> are straightforward: the pop functions that take a reference to a variable to receive the new value now have to dereference the stored pointer, 1 and 2, and the pop functions that return an std::shared_ptr<> instance can retrieve it from the queue, 3 and 4, before returning it to the caller.

If the data is held by std::shared_ptr<>, there’s an additional benefit: the allocation of the new instance can now be done outside the lock in push() 5, whereas in listing 6.2 it had to be done while holding the lock in pop(). Because memory allocation is typically quite an expensive operation, this can be beneficial for the performance of the queue, because it reduces the time the mutex is held, allowing other threads to perform operations on the queue in the meantime.

Just like in the stack example, the use of a mutex to protect the entire data structure limits the concurrency supported by this queue; although multiple threads might be blocked on the queue in various member functions, only one thread can be doing any work at a time. But part of this restriction comes from the use of std::queue<> in the implementation; by using the standard container you now have one data item that’s either protected or not. By taking control of the detailed implementation of the data structure, you can provide more fine-grained locking and allow a higher level of concurrency.

6.2.3. A thread-safe queue using fine-grained locks and condition variables

In listings 6.2 and 6.3 you have one protected data item (data_queue) and therefore one mutex. In order to use finer-grained locking, you need to look inside the queue at its constituent parts and associate one mutex with each distinct data item.

The simplest data structure for a queue is a singly linked list, as shown in figure 6.1. The queue contains a head pointer, which points to the first item in the list, and each item then points to the next item. Data items are removed from the queue by replacing the head pointer with the pointer to the next item and then returning the data from the old head.

Figure 6.1. A queue represented using a single-linked list



Items are added to the queue at the other end. In order to do this, the queue also contains a tail pointer, which refers to the last item in the list. New nodes are added by changing the next pointer of the last item to point to the new node and then updating the tail pointer to refer to the new item. When the list is empty, both the head and tail pointers are NULL.

The following listing shows a simple implementation of this queue based on a cut-down version of the interface to the queue in listing 6.2; you have only one try_pop() function and no wait_and_pop() because this queue only supports single-threaded use.

Listing 6.4. A simple single-threaded queue implementation

template<typename T>
class queue
{
private:
    struct node
    {
        T data;
        std::unique_ptr<node> next;
        node(T data_):
                data(std::move(data_))
        {}
    };
    std::unique_ptr<node> head;                     1
    node* tail;                                     2
public:
    queue(): tail(nullptr)
    {}
    queue(const queue& other)=delete;
    queue& operator=(const queue& other)=delete;
    std::shared_ptr<T> try_pop()
    {
        if(!head)
        {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> const res(
                std::make_shared<T>(std::move(head->data)));
        std::unique_ptr<node> const old_head=std::move(head);
        head=std::move(old_head->next);            3
        if(!head)
            tail=nullptr;
        return res;
    }
    void push(T new_value)
    {
        std::unique_ptr<node> p(new node(std::move(new_value)));
        node* const new_tail=p.get();
        if(tail)
        {
            tail->next=std::move(p);               4
        }
        else
        {
            head=std::move(p);                     5
        }
        tail=new_tail;                             6
    }
};
First off, note that listing 6.4 uses std::unique_ptr<node> to manage the nodes, because this ensures that they (and the data they refer to) get deleted when they’re no longer needed, without having to write an explicit delete. This ownership chain is managed from head, with tail being a raw pointer to the last node, as it needs to refer to a node already owned by std::unique_ptr<node>.

Although this implementation works fine in a single-threaded context, a couple of things will cause you problems if you try to use fine-grained locking in a multithreaded context. Given that you have two data items (head 1 and tail 2), you could in principle use two mutexes, one to protect head and one to protect tail, but there are a couple of problems with that.

The most obvious problem is that push() can modify both head 5 and tail 6, so it would have to lock both mutexes. This isn’t too much of a problem, although it’s unfortunate, because locking both mutexes would be possible. The critical problem is that both push() and pop() access the next pointer of a node: push() updates tail->next 4, and try_pop() reads head->next 3. If there’s a single item in the queue, then head==tail, so both head->next and tail->next are the same object, which therefore requires protection. Because you can’t tell if it’s the same object without reading both head and tail, you now have to lock the same mutex in both push() and try_pop(), so you’re no better off than before. Is there a way out of this dilemma?

Enabling concurrency by separating data

        You can solve this problem by preallocating a dummy node with no data to ensure that there’s always at least one node in the queue to separate the node being accessed at the head from that being accessed at the tail. For an empty queue, head and tail now both point to the dummy node rather than being NULL. This is fine, because try_pop() doesn’t access head->next if the queue is empty. If you add a node to the queue (so there’s one real node), then head and tail now point to separate nodes, so there’s no race on head->next and tail->next. The downside is that you have to add an extra level of indirection to store the data by pointer in order to allow the dummy nodes. The following listing shows how the implementation looks now.

Listing 6.5. A simple queue with a dummy node

template<typename T>
class queue
{
private:
    struct node
    {
        std::shared_ptr<T> data;                        1
        std::unique_ptr<node> next;
    };
    std::unique_ptr<node> head;
    node* tail;
public:
    queue():
            head(new node),tail(head.get())                 2
    {}
    queue(const queue& other)=delete;
    queue& operator=(const queue& other)=delete;
    std::shared_ptr<T> try_pop()
    {
        if(head.get()==tail)                            3
        {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> const res(head->data);       4
        std::unique_ptr<node> old_head=std::move(head);
        head=std::move(old_head->next);                 5
        return res;                                     6
    }
    void push(T new_value)
    {
        std::shared_ptr<T> new_data(
                std::make_shared<T>(std::move(new_value))); 7
        std::unique_ptr<node> p(new node);              8
        tail->data=new_data;                            9
        node* const new_tail=p.get();
        tail->next=std::move(p);
        tail=new_tail;
    }
};
The changes to try_pop() are fairly minimal. First, you’re comparing head against tail 3, rather than checking for NULL, because the dummy node means that head is never NULL. Because head is a std::unique_ptr<node>, you need to call head.get() to do the comparison. Second, because the node now stores the data by pointer 1, you can retrieve the pointer directly 4, rather than having to construct a new instance of T. The big changes are in push(): you must first create a new instance of T on the heap and take ownership of it in a std::shared_ptr<> 7 (note the use of std::make_shared to avoid the overhead of a second memory allocation for the reference count). The new node you create is going to be the new dummy node, so you don’t need to supply the new_value to the constructor 8. Instead, you set the data on the old dummy node to your newly allocated copy of the new_value 9. Finally, in order to have a dummy node, you have to create it in the constructor 2.

By now, I’m sure you’re wondering what these changes buy you and how they help with making the queue thread-safe. Well, push() now accesses only tail, not head, which is an improvement. try_pop() accesses both head and tail, but tail is needed only for the initial comparison, so the lock is short-lived. The big gain is that the dummy node means try_pop() and push() are never operating on the same node, so you no longer need an overarching mutex. You can have one mutex for head and one for tail. Where do you put the locks?

You’re aiming for the maximum number of opportunities for concurrency, so you want to hold the locks for the shortest possible length of time. push() is easy: the mutex needs to be locked across all accesses to tail, which means you lock the mutex after the new node is allocated 8, and before you assign the data to the current tail node 9. The lock then needs to be held until the end of the function.

try_pop() isn’t so easy. First off, you need to lock the mutex on head and hold it until you’re finished with head. This is the mutex to determine which thread does the popping, so you want to do that first. Once head is changed 5, you can unlock the mutex; it doesn’t need to be locked when you return the result 6. That leaves the access to tail needing a lock on the tail mutex. Because you need to access tail only once, you can just acquire the mutex for the time it takes to do the read. This is best done by wrapping it in a function. In fact, because the code that needs the head mutex locked is only a subset of the member, it’s clearer to wrap that in a function too. The final code is shown here.

Listing 6.6. A thread-safe queue with fine-grained locking

template<typename T>
class threadsafe_queue
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    std::mutex head_mutex;
    std::unique_ptr<node> head;
    std::mutex tail_mutex;
    node* tail;
    node* get_tail()
    {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }
    std::unique_ptr<node> pop_head()
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);

        if(head.get()==get_tail())
        {
            return nullptr;
        }
        std::unique_ptr<node> old_head=std::move(head);
        head=std::move(old_head->next);
        return old_head;
    }
public:
    threadsafe_queue():
            head(new node),tail(head.get())
    {}
    threadsafe_queue(const threadsafe_queue& other)=delete;
    threadsafe_queue& operator=(const threadsafe_queue& other)=delete;
    std::shared_ptr<T> try_pop()
    {
        std::unique_ptr<node> old_head=pop_head();
        return old_head?old_head->data:std::shared_ptr<T>();
    }
    void push(T new_value)
    {
        std::shared_ptr<T> new_data(
                std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);
        node* const new_tail=p.get();
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        tail->data=new_data;
        tail->next=std::move(p);
        tail=new_tail;
    }
};
Let’s look at this code with a critical eye, thinking about the guidelines listed in section 6.1.1. Before you look for broken invariants, you should be sure what they are:

tail->next==nullptr.
tail->data==nullptr.
head==tail implies an empty list.
A single element list has head->next==tail.
For each node x in the list, where x!=tail, x->data points to an instance of T and x->next points to the next node in the list. x->next==tail implies x is the last node in the list.
Following the next nodes from head will eventually yield tail.
On its own, push() is straightforward: the only modifications to the data structure are protected by tail_mutex, and they uphold the invariant because the new tail node is an empty node and data and next are correctly set for the old tail node, which is now the last real node in the list.

The interesting part is try_pop(). It turns out that not only is the lock on tail_mutex necessary to protect the read of tail itself, but it’s also necessary to ensure that you don’t get a data race reading the data from the head. If you didn’t have that mutex, it would be quite possible for a thread to call try_pop() and a thread to call push() concurrently, and there’d be no defined ordering on their operations. Even though each member function holds a lock on a mutex, they hold locks on different mutexes, and they potentially access the same data; all data in the queue originates from a call to push(), after all. Because the threads would be potentially accessing the same data without a defined ordering, this would be a data race, as you saw in chapter 5, and undefined behavior. Thankfully the lock on tail_mutex in get_tail() solves everything. Because the call to get_tail() locks the same mutex as the call to push(), there’s a defined order between the two calls. Either the call to get_tail() occurs before the call to push(), in which case it sees the old value of tail, or it occurs after the call to push(), in which case it sees the new value of tail and the new data attached to the previous value of tail.

It’s also important that the call to get_tail() occurs inside the lock on head_mutex. If it didn’t, the call to pop_head() could be stuck in between the call to get_tail() and the lock on the head_mutex, because other threads called try_pop() (and thus pop_head()) and acquired the lock first, preventing your initial thread from making progress:

std::unique_ptr<node> pop_head()                        1
{
node* const old_tail=get_tail();                    2
std::lock_guard<std::mutex> head_lock(head_mutex);

if(head.get()==old_tail)                            3
{
return nullptr;
}
std::unique_ptr<node> old_head=std::move(head);
head=std::move(old_head->next);                    4
return old_head;
}
1 This is a broken implementation.
2 Get old tail value outside lock on head_mutex
In this broken scenario, where the call to get_tail(0) 2 is made outside the scope of the lock, you might find that both head and tail have changed by the time your initial thread can acquire the lock on head_mutex, and not only is the returned tail node no longer the tail, but it’s no longer even part of the list. This could then mean that the comparison of head to old_tail 3 fails, even if head is the last node. Consequently, when you update head 4, you may end up moving head beyond tail and off the end of the list, destroying the data structure. In the correct implementation from listing 6.6, you keep the call to get_tail() inside the lock on head_mutex. This ensures that no other threads can change head, and tail only ever moves further away (as new nodes are added in calls to push()), which is perfectly safe. head can never pass the value returned from get_tail(), so the invariants are upheld.

Once pop_head() has removed the node from the queue by updating head, the mutex is unlocked, and try_pop() can extract the data and delete the node if there was one (and return a NULL instance of std::shared_ptr<> if not), safe in the knowledge that it’s the only thread that can access this node.

Next up, the external interface is a subset of that from listing 6.2, so the same analysis applies: there are no race conditions inherent in the interface.

Exceptions are more interesting. Because you’ve changed the data allocation patterns, the exceptions can now come from different places. The only operations in try_pop() that can throw exceptions are the mutex locks, and the data isn’t modified until the locks are acquired. Therefore try_pop() is exception-safe. On the other hand, push() allocates a new instance of T on the heap and a new instance of node, either of which might throw an exception. But both of the newly allocated objects are assigned to smart pointers, so they’ll be freed if an exception is thrown. Once the lock is acquired, none of the remaining operations in push() can throw an exception, so again you’re home and dry and push() is exception-safe too.

Because you haven’t changed the interface, there are no new external opportunities for deadlock. There are no internal opportunities, either; the only place that two locks are acquired is in pop_head(), which always acquires the head_mutex, and then the tail_mutex, so this will never deadlock.

The remaining question concerns the possibilities for concurrency. This data structure has considerably more scope for concurrency than that from listing 6.2, because the locks are more fine-grained and more is done outside the locks. For example, in push(), the new node and new data item are allocated with no locks held. This means that multiple threads can be allocating new nodes and data items concurrently without a problem. Only one thread can add its new node to the list at a time, but the code to do so is only a few simple pointer assignments, so the lock isn’t held for much time at all compared to the std::queue<>-based implementation where the lock is held around all the memory allocation operations internal to the std::queue<>.

Also, try_pop() holds the tail_mutex for only a short time, to protect a read from tail. Consequently, almost the entirety of a call to try_pop() can occur concurrently with a call to push(). Also, the operations performed while holding the head_mutex are quite minimal; the expensive delete (in the destructor of the node pointer) is outside the lock. This will increase the number of calls to try_pop() that can happen concurrently; only one thread can call pop_head() at a time, but multiple threads can then delete their old nodes and return the data safely.

Waiting for an item to pop

OK, so listing 6.6 provides a thread-safe queue with fine-grained locking, but it supports only try_pop() (and only one overload at that). What about the handy wait_and_pop() functions back in listing 6.2? Can you implement an identical interface with your fine-grained locking?

The answer is yes, but the real question is how. Modifying push() is easy: add the data_cond.notify_one() call at the end of the function, like in listing 6.2. It’s not quite that simple; you’re using fine-grained locking because you want the maximum possible amount of concurrency. If you leave the mutex locked across the call to notify_one() (as in listing 6.2), then if the notified thread wakes up before the mutex has been unlocked, it will have to wait for the mutex. On the other hand, if you unlock the mutex before you call notify_one(), then the mutex is available for the waiting thread to acquire when it wakes up (assuming no other thread locks it first). This is a minor improvement, but it might be important in some cases.

wait_and_pop() is more complicated, because you have to decide where to wait, what the predicate is, and which mutex needs to be locked. The condition you’re waiting for is “queue not empty,” which is represented by head!=tail. Written like that, it would require both head_mutex and tail_mutex to be locked, but you’ve already decided in listing 6.6 that you only need to lock tail_mutex for the read of tail and not for the comparison itself, so you can apply the same logic here. If you make the predicate head!=get_tail(), you only need to hold head_mutex, so you can use your lock on that for the call to data_cond.wait(). Once you’ve added the wait logic, the implementation is the same as try_pop().

The second overload of try_pop() and the corresponding wait_and_pop() overload require careful thought. If you replace the return of std::shared_ptr<> retrieved from old_head with a copy assignment to the value parameter, there’s a potential exception-safety issue. At this point, the data item has been removed from the queue and the mutex unlocked; all that remains is to return the data to the caller. But if the copy assignment throws an exception (as it might), the data item is lost because it can’t be returned to the queue in the same place.

If the actual type T used for the template argument has a no-throw move-assignment operator or a no-throw swap operation, you could use that, but you’d prefer a general solution that could be used for any type T. In this case, you have to move the potential throwing operation inside the locked region before the node is removed from the list. This means you need an extra overload of pop_head() that retrieves the stored value prior to modifying the list.

In comparison, empty() is trivial: lock head_mutex and check for head== get_tail() (see listing 6.10). The final code for the queue is shown in listings 6.7, 6.8, 6.9, and 6.10.

Listing 6.7. A thread-safe queue with locking and waiting: internals and interface

template<typename T>
class threadsafe_queue
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    std::mutex head_mutex;
    std::unique_ptr<node> head;
    std::mutex tail_mutex;
    node* tail;
    std::condition_variable data_cond;
public:
    threadsafe_queue():
            head(new node),tail(head.get())
    {}
    threadsafe_queue(const threadsafe_queue& other)=delete;
    threadsafe_queue& operator=(const threadsafe_queue& other)=delete;
    std::shared_ptr<T> try_pop();
    bool try_pop(T& value);
    std::shared_ptr<T> wait_and_pop();
    void wait_and_pop(T& value);
    void push(T new_value);
    bool empty();
};
Pushing new nodes onto the queue is fairly straightforward—the implementation (shown in the following listing) is close to that shown previously.

Listing 6.8. A thread-safe queue with locking and waiting: pushing new values

template<typename T>
void threadsafe_queue<T>::push(T new_value)
{
    std::shared_ptr<T> new_data(
            std::make_shared<T>(std::move(new_value)));
    std::unique_ptr<node> p(new node);
    {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        tail->data=new_data;
        node* const new_tail=p.get();
        tail->next=std::move(p);
        tail=new_tail;
    }
    data_cond.notify_one();
}

template<typename T>
class threadsafe_queue
{
private:
    node* get_tail()
    {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }
    std::unique_ptr<node> pop_head()                             1
    {
        std::unique_ptr<node> old_head=std::move(head);
        head=std::move(old_head->next);
        return old_head;
    }
    std::unique_lock<std::mutex> wait_for_data()                 2
    {
        std::unique_lock<std::mutex> head_lock(head_mutex);
        data_cond.wait(head_lock,[&]{return head.get()!=get_tail();});
        return std::move(head_lock);                             3
    }
    std::unique_ptr<node> wait_pop_head()
    {
        std::unique_lock<std::mutex> head_lock(wait_for_data()); 4
        return pop_head();
    }
    std::unique_ptr<node> wait_pop_head(T& value)
    {
        std::unique_lock<std::mutex> head_lock(wait_for_data()); 5
        value=std::move(*head->data);
        return pop_head();
    }
public:
    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_ptr<node> const old_head=wait_pop_head();
        return old_head->data;
    }
    void wait_and_pop(T& value)
    {
        std::unique_ptr<node> const old_head=wait_pop_head(value);
    }
};

template<typename T>
class threadsafe_queue
{
private:
    std::unique_ptr<node> try_pop_head()
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if(head.get()==get_tail())
        {
            return std::unique_ptr<node>();
        }
        return pop_head();
    }
    std::unique_ptr<node> try_pop_head(T& value)
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if(head.get()==get_tail())
        {
            return std::unique_ptr<node>();
        }
        value=std::move(*head->data);
        return pop_head();
    }
public:
    std::shared_ptr<T> try_pop()
    {
        std::unique_ptr<node> old_head=try_pop_head();
        return old_head?old_head->data:std::shared_ptr<T>();
    }
    bool try_pop(T& value)
    {
        std::unique_ptr<node> const old_head=try_pop_head(value);
        return old_head;
    }
    bool empty()
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        return (head.get()==get_tail());
    }
};
#endif