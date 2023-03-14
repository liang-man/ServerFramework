#ifndef __SYLAR_THREAD_H
#define __SYLAR_THREAD_H

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>

namespace sylar {

// 信号量 
// 常用到消息队列里，比如服务器来消息了，收到消息后塞到消息队列里面，另外一组线程去消费这个消息队列，一直去wait，如果wait到了，那就把消息取出来；
// 如果没有wait到，那就说明没消息，这样就会一直陷入这个wait等待里面去
class Semaphore {
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    void wait();
    void notify();
private:
    Semaphore(const Semaphore &) = delete;
    Semaphore(const Semaphore &&) = delete;
    Semaphore &operator=(const Semaphore &) = delete;
private:
    sem_t m_semaphore;
};

// 互斥量一般都在一个局部的范围，首先让它锁，锁完之后要让他解锁，为了防止我们漏掉解锁，我们一般是写一个类，通过类的构造函数加锁，类的析构函数来解锁
// 锁的种类有很多，但方法都无非是那几个，所以我们定义一个通用的模板类去锁他们，这样传进来都是一个互斥量，但是是什么类型的互斥量我们不关心它
// 我们这是模板锁，只要提供锁和解锁的方法就行了
template<class T>
struct ScopedLockImpl {
public:
    ScopedLockImpl(T &mutex) : m_mutex(mutex) {
        m_mutex.lock();   // 加锁
        m_locked = true;  // 设置状态
    }

    ~ScopedLockImpl() {
        unlock();
    }

    void lock() {
        if (!m_locked) {  // 上锁之前先判断一下，没上锁才给加锁，防止死锁 
            m_mutex.lock();
            m_locked = true;
        }
    }

    void unlock() {
        if (m_locked) {    // 如果已经上锁了，才解锁
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T &m_mutex;
    bool m_locked;
};

template<class T>
struct ReadScopedLockImpl {
public:
    ReadScopedLockImpl(T &mutex) : m_mutex(mutex) {
        m_mutex.rdlock();   // 加锁
        m_locked = true;  // 设置状态
    }

    ~ReadScopedLockImpl() {
        unlock();
    }

    void lock() {
        if (!m_locked) {  // 上锁之前先判断一下，没上锁才给加锁，防止死锁 
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    void unlock() {
        if (m_locked) {    // 如果已经上锁了，才解锁
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T &m_mutex;
    bool m_locked;
};

template<class T>
struct WriteScopedLockImpl {
public:
    WriteScopedLockImpl(T &mutex) : m_mutex(mutex) {
        m_mutex.wrlock();   // 加锁
        m_locked = true;  // 设置状态
    }

    ~WriteScopedLockImpl() {
        unlock();
    }

    void lock() {
        if (!m_locked) {  // 上锁之前先判断一下，没上锁才给加锁，防止死锁 
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    void unlock() {
        if (m_locked) {    // 如果已经上锁了，才解锁
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T &m_mutex;
    bool m_locked;
};

// 普通的互斥量
// 在读和写的频率差不多的时候，不分读写的互斥量会好一点
class Mutex {
public:
    typedef ScopedLockImpl<Mutex> Lock;

    Mutex() {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    } 

    void lock() {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }
private:
    pthread_mutex_t m_mutex;
};

// 空的锁，什么都不干，为了测试，这样测试起来就很方便
class NullMutex {
public:
    typedef ScopedLockImpl<NullMutex> Lock;
    NullMutex() {}
    ~NullMutex() {}
    void lock() {}
    void unlock() {}
};

// 分读和写的互斥量
// 大并发的时候，分读和写会好一点
class RWMutex {
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    RWMutex() {
        pthread_rwlock_init(&m_lock, nullptr);
    }

    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
    }

    void rdlock() {
        pthread_rwlock_rdlock(&m_lock);
    }

    void wrlock() {
        pthread_rwlock_wrlock(&m_lock);
    }

    void unlock() {
        pthread_rwlock_unlock(&m_lock);
    }
private:
    pthread_rwlock_t m_lock;
};

class NullRWMutex {
public:
    typedef ReadScopedLockImpl<NullRWMutex> ReadLock;
    typedef WriteScopedLockImpl<NullRWMutex> WriteLock;
    NullRWMutex() {}
    ~NullRWMutex() {}
    void lock() {}
    void unlock() {}
};

class SpinLock {
public:
    typedef ScopedLockImpl<SpinLock> Lock;

    SpinLock() {
        pthread_spin_init(&m_mutex, 0);
    }

    ~SpinLock() {
        pthread_spin_destroy(&m_mutex);
    } 

    void lock() {
        pthread_spin_lock(&m_mutex);
    }

    void unlock() {
        pthread_spin_unlock(&m_mutex);
    }
private:
    pthread_spinlock_t m_mutex;
};

// spinlock就是用cas实现的，CASLock没有SpinLock性能高,SpinLock的cpu消耗要比CASLock低一些
class CASLock {
public:
    typedef ScopedLockImpl<CASLock> Lock;

    CASLock() {
        m_mutex.clear();
    }

    ~CASLock() {

    }

    void lock() {
        while (std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
    }   

    void unlock() {
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }
private:
    volatile std::atomic_flag m_mutex;
};

class Thread {
public:
    typedef std::shared_ptr<Thread> ptr;   // 内存分配尽量少用裸指针，尽量用智能指针，这样在内存泄露方面风险就会少很多
    Thread (std::function<void()> cb, const std::string &name);
    ~Thread();

    pid_t getId() const { return m_id; }
    const std::string &getName() const { return m_name; }   // 需要用到当前线程时用的

    void join();

    static Thread *GetThis();    // 拿到自己这个线程，从而做一些操作  为什么要用静态方法？
    static const std::string &GetName();    // 给日志用的  为什么要用静态方法？
    static void SetName(const std::string &name);   // 写方法，比如主线程不是我们自己创建的，如果name只能在我们自己创建的线程中拿到的话，就不是很合适了
private:
    // 线程库禁止默认拷贝
    Thread (const Thread &) = delete;
    Thread (const Thread &&) = delete;
    Thread &operator=(const Thread &) = delete;

    static void *run(void *arg);
private:
    pid_t m_id = -1;           // 线程id, 用pthread的方式, 我们的线程id是linux在top命令里显示的线程号，这样在调试时输出的线程id跟实际运行的线程id是一致的
    pthread_t m_thread = 0;   // 线程，用pthread的方式，std的线程里的功能还是有点少，不满足我们的需求，比如我们还需要线程名称
    std::function<void()> m_cb;
    std::string m_name;   // 线程名称

    Semaphore m_semaphore;
};

}

#endif