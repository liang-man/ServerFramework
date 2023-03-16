#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static thread_local Scheduler *t_scheduler = nullptr;   // 协程调度器指针

static thread_local Fiber *t_fiber = nullptr;           

// 创建一个协程，创建的协程执行run方法，但这个协程还未被执行起来
Scheduler::Scheduler(size_t threads = 1, bool use_caller = true, const std::string &name = "") : m_name(name)
{
    SYLAR_ASSERT(threads > 0);
    if (use_caller) {
        sylar::Fiber::GetThis();   // 如果没有main协程的话会初始化一个
        --threads;
        SYLAR_ASSERT(GetThis() == nullptr);   // 如果GetThis()不为空，说明这个线程里已经存在了一个协程调度器，不能有第二个
        t_scheduler = this;        // 协程调度器等于this
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this)));
        sylar::Thread::SetName(m_name);
        // 在一个线程里声明一个调度器，当把这个线程放入这个调度器的后，调度器的主协程不再是这个线程的主协程，而是执行run方法的协程作为主协程
        t_fiber = m_rootFiber.get();
        m_rootThreadId = sylar::GetThreadId();
        m_threadIds.push_back(m_rootThreadId);
    } else {
        m_rootThreadId = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler()
{
    SYLAR_ASSERT(m_stopping);
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler *Scheduler::GetThis()
{
    return t_scheduler;
}

Fiber *Scheduler::GetMainFiber()
{   
    return t_fiber;
} 

// 启动线程池
void Scheduler::start()
{
    MutexType::Lock lock(m_mutex);
    if (!m_stopping) {
        return;
    }
    m_stopping = false;
    SYLAR_ASSERT(m_threads.empty());
    m_threads.resize(m_threadCount);
    for (size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
}

/*
 * stop分两种情况：
 * 1: scheduler用了usecaller，就一定要在创建了scheduler的那个线程中stop
 * 2: scheduler没有用usecallser，就可以在任意非它自己的线程中执行stop
*/
void Scheduler::stop()
{
    m_autoStop = true;
    if (m_rootFiber && m_threadCount == 0 
            && (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT)) {
        SYLAR_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;
        if (stopping()) {

        }
    }
    bool exit_on_this_fiber = false;
    if (m_rootThreadId != -1) {
        SYLAR_ASSERT(GetThis() == this);
    } else {
        SYLAR_ASSERT(GetThis() != this);
    }
    m_stopping = true;
    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle();    // 线程唤醒 类似于信号量，tickle一下就唤醒，唤醒回来后就自己结束了
    }
    if (m_rootFiber) {
        tickle();
    }
    if (stopping()) {
        return;
    }
}

void Scheduler::setThis()
{
    t_scheduler = this;
}

void Scheduler::run()
{
    setThis();
    if (sylar::GetThreadId() != m_rootThreadId) {
        t_fiber = Fiber::GetThis().get();
    }
    Fiber::ptr idle_fiber(new Fiber());
}

bool Scheduler::stopping()
{

}

}