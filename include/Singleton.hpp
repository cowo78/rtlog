#pragma once

#include <map>
#include <memory>
#include <string>

template <typename T>
class Singleton
{
public:
    typedef T derived_type;

    template<typename... Args>
    static inline void initialize(Args& ... args)
    {
        derived_type* p(new derived_type(args...));
        instance.reset(p);
        atomic_rtlogger.store(p, std::memory_order_seq_cst);
    }
    static inline void destroy() { instance.reset(); }
    static inline T& get() { return *instance; }

protected:
    Singleton() {};
    ~Singleton() {};

private:
    Singleton(const Singleton&);
    Singleton& operator=(const Singleton&);

    static std::unique_ptr<T> instance;
    static std::atomic<T*> atomic_rtlogger;
};

template<typename T> std::unique_ptr<T> Singleton<T>::instance;
template<typename T> std::atomic<T*> Singleton<T>::atomic_rtlogger;
