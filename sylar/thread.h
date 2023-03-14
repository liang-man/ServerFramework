#ifndef __SYLAR_THREAD_H
#define __SYLAR_THREAD_H

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

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
    Semaphore operator=(const Semaphore &) = delete;
private:
    sem_t m_semaphore;
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
    Thread operator=(const Thread &) = delete;

    static void *run(void *arg);
private:
    pid_t m_id = -1;           // 线程id, 用pthread的方式, 我们的线程id是linux在top命令里显示的线程号，这样在调试时输出的线程id跟实际运行的线程id是一致的
    pthread_t m_thread = 0;   // 线程，用pthread的方式，std的线程里的功能还是有点少，不满足我们的需求，比如我们还需要线程名称
    std::function<void()> m_cb;
    std::string m_name;   // 线程名称
};

}

#endif