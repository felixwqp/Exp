//
// Created by 王起鹏 on 12/12/21.
//

#ifndef PLAYGROUND_HANDLER_H
#define PLAYGROUND_HANDLER_H
#include <iostream>

template <typename T>
struct rm_ptr {
    typedef T type;
};

template <typename T>
struct rm_ptr<T*> {
    typedef T type;
};

template <typename T>
struct rm_ptr<const T*> {
    typedef const T type;
};



template <typename R, typename T, R (*F)(T)>
struct deleter {
    template <typename U>
    void operator()(U& ptr) const
    {
        (*F)(ptr);
    }
};

template <typename T, typename Deleter = deleter<void, void*, &::free>>
class handle {
    struct dummy;
    T _val;
    bool _empty;

#ifdef BACKWARD_ATLEAST_CXX11
    handle(const handle&) = delete;
    handle& operator=(const handle&) = delete;
#endif

public:
    ~handle()
    {
        if (!_empty) {
            Deleter()(_val);
        }
    }

    explicit handle() : _val(), _empty(true) {}
    explicit handle(T val) : _val(val), _empty(false)
    {
        if (!_val)
            _empty = true;
    }

#ifdef BACKWARD_ATLEAST_CXX11
    handle(handle&& from) : _empty(true) { swap(from); }
    handle& operator=(handle&& from)
    {
        swap(from);
        return *this;
    }
#else
    explicit handle(const handle& from) : _empty(true)
    {
        // some sort of poor man's move semantic.
        swap(const_cast<handle&>(from));
    }
    handle& operator=(const handle& from)
    {
        // some sort of poor man's move semantic.
        swap(const_cast<handle&>(from));
        return *this;
    }
#endif

    void reset(T new_val)
    {
        handle tmp(new_val);
        swap(tmp);
    }

    void update(T new_val)
    {
        _val = new_val;
        _empty = static_cast<bool>(new_val);
    }

    operator const dummy*() const
    {
        if (_empty) {
            return nullptr;
        }
        return reinterpret_cast<const dummy*>(_val);
    }
    T get() { return _val; }
    T release()
    {
        _empty = true;
        return _val;
    }
    void swap(handle& b)
    {
        using std::swap;
        swap(b._val, _val);     // can throw, we are safe here.
        swap(b._empty, _empty); // should not throw: if you cannot swap two
        // bools without throwing... It's a lost cause anyway!
    }

    T& operator->() { return _val; }
    const T& operator->() const { return _val; }

    typedef typename rm_ptr<T>::type& ref_t;
    typedef const typename rm_ptr<T>::type& const_ref_t;
    ref_t operator*() { return *_val; }
    const_ref_t operator*() const { return *_val; }
    ref_t operator[](size_t idx) { return _val[idx]; }

    // Watch out, we've got a badass over here
    T* operator&()
    {
        _empty = false;
        return &_val;
    }
};

#endif //PLAYGROUND_HANDLER_H
