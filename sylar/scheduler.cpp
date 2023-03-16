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
 * stop不能立马退出，要等协程调度器所有任务执行完在退出
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
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;   // 回调函数，function的协程
    FiberAndThread ft;
    while (true) {
        ft.reset();
        bool tickle_me = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while (it != m_fibers.end()) {
                if (it->threadId != -1 && it->threadId != sylar::GetThreadId()) {
                    ++it;
                    tickle_me = true;   // 自己消耗了一个信号，也应该再发起一个信号，让其他线程再去有唤醒的机会，不过唤醒的线程是无序的，不能指定线程唤醒（思考如何优化，可以唤醒指定线程？）
                    continue;
                }
                SYLAR_ASSERT(it->fiber || it->cb);
                if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }
                ft = *it;
                m_fibers.erase(it);
            }
        }
        if (tickle_me) {
            tickle();
        }
        if (ft.fiber && (ft.fiber->getState() != Fiber::TERM || ft.fiber->getState() != Fiber::EXCEPT)) {
            ++m_activeThreadCount;
            ft.fiber->swapIn();
            --m_activeThreadCount;
            if (ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            } else if (ft.fiber->getState() != Fiber::READY
                    && ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->setState(Fiber::HOLD);
            }
            ft.reset();
        } else if (ft.cb) {
            if (cb_fiber) {
                cb_fiber->reset(&ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            ++m_activeThreadCount;
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if (cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();    // 智能指针.reset() 和 ->reset(nullptr)的区别？
            } else if (cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);    // 好像是不会引起析构?
            } else { // if (cb_fiber->getState() != Fiber::TERM) {
                ft.fiber->setState(Fiber::HOLD);
                cb_fiber.reset();
            }
        } else {   // 当事情做完了，去ilde一下
            if (idle_fiber->getState() == Fiber::TERM) {
                break;
            }
            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if (idle_fiber->getState() != Fiber::TERM || idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->setState(Fiber::HOLD);
            }
        }
    }
}

bool Scheduler::stopping()
{

}

}