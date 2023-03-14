#ifndef __SYLAR_THREAD_H
#define __SYLAR_THREAD_H

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>

namespace sylar {

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
    static void *run(void *arg);
private:
    // 线程库禁止默认拷贝
    Thread (const Thread &) = delete;
    Thread (const Thread &&) = delete;
    Thread operator=(const Thread &) = delete;
private:
    pid_t m_id;           // 线程id, 用pthread的方式, 我们的线程id是linux在top命令里显示的线程号，这样在调试时输出的线程id跟实际运行的线程id是一致的
    pthread_t m_thread;   // 线程，用pthread的方式，std的线程里的功能还是有点少，不满足我们的需求，比如我们还需要线程名称
    std::function<void()> m_cb;
    std::string m_name;   // 线程名称
};

}

#endif