#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

static thread_local Fiber *t_fiber = nullptr;   // 用线程局部变量，拿到当前协程或main协程
static thread_local Fiber::ptr t_threadFiber = nullptr;   // 主协程

// 定义一个协程栈大小
static ConfigVar<uint32_t>::ptr g_fiber_stack_size = 
    Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

// 内存分配器
class MallocStackAllocator {
public:
    static void *Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void *vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId()
{
    if (t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

// 把当前线程的上下文赋给我们这个类
// 私有的构造函数，也可能会被静态函数调用，很多单例都是这么写的（网友弹幕）
// 默认构造函数是这个线程的主协程，主协程是没有栈的
Fiber::Fiber()
{
    m_state = EXEC;   // 第一个创建的协程就是main协程，处在正在执行中的状态
    SetThis(this);

    if (getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }
    ++s_fiber_count;

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber";
}

// 真正的创建一个协程，需要分配一个栈空间，每个协程都有一个独立的栈，所以每个协程都是在一个固定大小的栈上执行它要执行的函数
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    : m_id(++s_fiber_id), m_cb(cb)
{
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();   // 如果初始化给了0，那我就以配置为主；否则你说给多少就是多少

    m_stack = StackAllocator::Alloc(m_stacksize);
    if (getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;    // 这个上下文关联了上下文
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    if (!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
}

// 可以做一个最小堆回收fiber io（网友弹幕）
// 协程结束后，回收内存
Fiber::~Fiber()
{
    --s_fiber_count;
    if (m_stack) {   // main协程的时候是不会有栈的，所以只要我们判断有栈的话，能被析构的话，那肯定是结束了或者还在初始化(还没跑起来就结束了)
        SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
        StackAllocator::Dealloc(m_stack, m_stacksize);     // 回收栈
    } else {         // 说明是main协程
        // 确认一下
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state == EXEC);

        Fiber *cur = t_fiber;   
        if (cur == this) {    // 看一下fiber是不是自己，其实是等于自己的
            SetThis(nullptr);
        }
    }

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id;
}

// 一个协程执行完了，但是对应的内存没释放，那我就可以基于这个内存重新初始化，重新创建一个新的协程
void Fiber::reset(std::function<void()> cb)
{
    SYLAR_ASSERT(m_stack);   // 主协程是没有栈的
    SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);   // 条件为真就继续运行
    m_cb = cb;   // 重新置一下回调函数
    if (getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;    // 这个上下文关联了上下文
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}

void Fiber::call()
{
    SetThis(this);
    m_state = EXEC;
    // SYLAR_ASSERT(GetThis() == t_threadFiber);
    SYLAR_LOG_ERROR(g_logger) << getId();
    if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

void Fiber::back()
{
    SetThis(t_threadFiber.get());
    if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    } 
}

// 正常情况下操作对象一定是子协程，不是main协程
// 从线程的main协程swap到当前协程
void Fiber::swapIn()
{
    SetThis(this);
    SYLAR_ASSERT(m_state != EXEC);    // 条件为真就继续运行
    m_state = EXEC;
    // if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {     // 与协程调度器功能有冲突
    if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {    
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

// 从当前协程swap到main协程
#if 1
void Fiber::swapOut()
{
    // SetThis(t_threadFiber.get());    // 加入协程调度器功能后产生bug
    SetThis(Scheduler::GetMainFiber());
    // if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {    // 加入协程调度器功能后产生bug
    if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}
#endif

// 这里需要来回做切换，比较频繁，想想怎么优化，不用if else判断，那样速度就会快很多
// 优化措施就像call()一样，把else里的部分单独拎出来作为back()函数
// 既然这样优化完， 这里就没必要加判断了，还是修改为原来的
#if 0
void Fiber::swapOut()
{
    // SetThis(t_threadFiber.get());
    // if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {    // 与协程调度器功能有冲突
    if (t_fiber != Scheduler::GetMainFiber()) {
        SetThis(Scheduler::GetMainFiber());
        if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    } else {    // 跟真正的main协程切换
        SetThis(t_threadFiber.get());
        if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext");
        } 
    }
}
#endif

void Fiber::SetThis(Fiber *f)
{
    t_fiber = f;
}

Fiber::ptr Fiber::GetThis()
{
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);   // static函数会调用默认构造函数
    SYLAR_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

void Fiber::YieldToReady()
{
    Fiber::ptr cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}

void Fiber::YieldToHold()
{
    Fiber::ptr cur = GetThis();
    cur->m_state = HOLD;
    cur->swapOut();
}

uint64_t Fiber::TotalFibers()
{
    return s_fiber_count;
}

void Fiber::MainFunc()
{
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;   // 为什么要置为nullptr，与function有关，会使引用计数加1  看P28 23'48''
        cur->m_state = TERM;
    } catch (std::exception &ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber except: " << ex.what() << " Fiber_id=" << cur->getId() << std::endl << sylar::BacktraceToString();      
    } catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber except: " << " Fiber_id=" << cur->getId() << std::endl << sylar::BacktraceToString();;
    }

    auto raw_ptr = cur.get();   // 裸指针
    cur.reset();
    raw_ptr->swapOut();

    SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

void Fiber::CallerMainFunc()
{
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;   // 为什么要置为nullptr，与function有关，会使引用计数加1  看P28 23'48''
        cur->m_state = TERM;
    } catch (std::exception &ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber except: " << ex.what() << " Fiber_id=" << cur->getId() << std::endl << sylar::BacktraceToString();      
    } catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber except: " << " Fiber_id=" << cur->getId() << std::endl << sylar::BacktraceToString();;
    }

    auto raw_ptr = cur.get();   // 裸指针
    cur.reset();
    raw_ptr->back();    // 不用swapOut(),用back()，为了优化,不要if else判断

    SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

}