#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <ucontext.h>
#include <memory>
#include <functional>
#include "thread.h"

namespace sylar {

// 要把这个类作为智能指针，那就要继承enable_shared_from_this，其里面有个方法，可以获取当前类的智能指针
// 继承enable_shared_from_this类的对象就不可以在栈上创建对象，查一下为什么？看视频P27 6'50''
class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT,    // 初始化
        HOLD,    // 跑的过程中，人工的hold住
        EXEC,    // 正在执行
        TERM,
        READY,
        EXCEPT   // 异常状态
    };
private:
    Fiber();   // 不允许创建默认构造函数，所以把它变成私有的

public:
    Fiber(std::function<void()> cb, size_t stacksize = 0);    // functional解决了很多函数指针不适用的场景
    ~Fiber();

    void reset(std::function<void()> cb);  // 重置协程函数，并重置状态，只能在INIT或TERM状态
    void swapIn();   // 切换到当前协程执行，自己开始执行了
    void swapOut();  // 把当前协程切换到后台，我不执行了，让出控制权

    uint64_t getId() const { return m_id; }
public:
    static void SetThis(Fiber *f);  // 设置当前协程
    static Fiber::ptr GetThis();           // 拿到自己的协程
    static void YieldToReady();     // 协程切换到后台，并设置为ready的状态
    static void YieldToHold();      // 协程切换到后台，并设置为hold的状态

    static uint64_t TotalFibers();  // 总协程数

    static void MainFunc();
    static uint64_t GetFiberId();
private:
    uint64_t m_id = 0;
    uint64_t m_stacksize = 0;
    State m_state = INIT;

    ucontext_t m_ctx;
    void *m_stack = nullptr;

    std::function<void()> m_cb;
};

}

#endif