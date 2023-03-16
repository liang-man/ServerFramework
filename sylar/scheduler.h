#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include "fiber.h"
#include "thread.h"

namespace sylar {

class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    /*
     * threads: 线程数，默认是1个，可以自己创建多个线程
     * use_caller: 在哪个线程执行了Scheduler构造函数的时候，设置为true，意味着要把这个线程纳入到线程调度器里面来
     * name: 线程的名称
    */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string &name = "");
    virtual ~Scheduler();    // 这个Scheduler类只是基类，后面会根据具体的特性，实现不同的子类

    const std::string &getName() const { return m_name; }

    static Scheduler *GetThis();   // 获取当前协程调度器
    static Fiber *GetMainFiber();  // 需要一个main协程来管理调度器

    void start();     // 启动线程池
    void stop();     

    template<class FiberOrCb> 
    void schedule(FiberOrCb fc, int threadId = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lokc(&m_mutex);
            need_tickle = scheduleNoLock(fc, threadId);
        }
        if (need_tickle) {
            tickle();
        }
    }

    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(&m_mutex);   // 锁一次保证这组任务是连续的，在一个消息队列里
            while (begin != end) {
                // 用的是指针，取地址，地址的话就会把里面的东西swap掉
                need_tickle = scheduleNoLock(&*begin) || need_tickle;
            }
        }
        if (need_tickle) {
            tickle();
        }
    }
protected:
    virtual void tickle();
    void run();     // 协程调度器真正在执行调度的方法
    virtual bool stopping();

    void setThis();
private:
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int threadId) {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, threadId);
        if (ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }
private:
    // 需要执行的协程对象
    struct FiberAndThread {
        Fiber::ptr fiber;           
        std::function<void()> cb;   // 回调
        int threadId;               // 线程id，协程调度器需要指定协程在哪个线程上执行，为了这个功能

        FiberAndThread(Fiber::ptr f, int thr) : fiber(f), threadId(thr) {}
        FiberAndThread(Fiber::ptr *f, int thr) : threadId(thr) { 
            // 协程智能指针的指针
            // 不推荐使用智能指针的指针，后期要重构一下这块(网友弹幕)
            // 为了引用计数方面的考虑
            fiber.swap(*f);   
        }
        FiberAndThread(std::function<void()> f, int thr) : cb(f), threadId(thr) {}
        FiberAndThread(std::function<void()> *f, int thr) : threadId(thr) {
            cb.swap(*f);
        }
        FiberAndThread() : threadId(-1) {}

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            threadId = -1;
        }
    };
private:
    MutexType m_mutex;   // 互斥量
    std::vector<Thread::ptr> m_threads;   // 线程池
    std::list<FiberAndThread> m_fibers;   // 即将要执行或计划执行的协程队列
    Fiber::ptr m_rootFiber;               // 主协程
    std::string m_name;
protected:
    std::vector<int> m_threadIds;
    size_t m_threadCount = 0;
    size_t m_activeThreadCount = 0;
    size_t m_idleThreadCount = 0;
    bool m_stopping = true;
    bool m_autoStop = false;       // 是否主动停止
    int m_rootThreadId = 0;    // usecaller的id
};

}

#endif