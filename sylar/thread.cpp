#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar {

// 因为我们要拿到当前线程，所以用一个线程局部变量，指向的就是当前的线程
static thread_local Thread *t_thread = nullptr;
// 返回名称的时候，直接返回这个值会快一点，因为不需要做转换了
static thread_local std::string t_thread_name = "UNKNOW";

// 系统的日志都统一叫system
// 这里又出现不同文件静态变量初始化顺序的问题了？g_logger应该加在log.cpp里确定初始化顺序？（来自弹幕）
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Semaphore::Semaphore(uint32_t count)
{
    if (sem_init(&m_semaphore, 0, count)) {
        throw std::logic_error("sem_init error");
    }
}

Semaphore::~Semaphore()
{
    sem_destroy(&m_semaphore);
}

void Semaphore::wait()
{
    if (sem_wait(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
}

void Semaphore::notify()
{
    if (sem_post(&m_semaphore)) {
        throw std::logic_error("sem_post error");
    }
}

Thread *Thread::GetThis()
{
    return t_thread;
}  

const std::string &Thread::GetName()
{
    return t_thread_name;
} 

void Thread::SetName(const std::string &name)
{
    if (t_thread) {
        t_thread->m_name = name;   // 如果线程有的话，直接改线程名称
    } 
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string &name) : m_cb(cb), m_name(name)
{
    if (name.empty()) {
        m_name = "UNKNOW";
    } 
    // 创建的这个线程，有可能在这个构造函数返回的时候它还没有开始执行，有的时候可能已经执行，顺序不确定
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
            << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
    m_semaphore.wait();   // 所以就在这里等待，如果这个线程没有运行起来，就不出这个构造函数
}

// 问：线程类的析构函数如何设计？
Thread::~Thread()
{
    if (m_thread) {
        pthread_detach(m_thread);
    }
}

void Thread::join()
{
    if (m_thread) {   // 如果还有值，就可以join
        int rt = pthread_join(m_thread, nullptr);
        if (rt) {
            SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;  // join完了之后清空，就不需要了
    }
}

void *Thread::run(void *arg)
{
    Thread *thread = (Thread *)arg;   // 拿到的参数肯定是一个线程的参数，所以先要转一下
    t_thread = thread;   // 把线程局部变量设置了
    t_thread_name = thread->m_name;       // 设置t_thread_name  找了1个小时的bug(三个线程一起跑时，显示每个线程的名字为UNKNOW)，就是因为这个里的t_thread_name名字没有设置
    thread->m_id = sylar::GetThreadId();
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    cb.swap(thread->m_cb);   // 防止当这个函数里有智能指针的时候，防止它的引用会出现在日志不会被释放掉，这样做至少少了一个引用

    thread->m_semaphore.notify();    // 等线程初始化好了之后把它唤醒 这样保证了线程创建好之后，一定是跑起来的

    cb();
    return 0;
}

}