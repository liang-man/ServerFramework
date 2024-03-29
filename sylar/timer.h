#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include <set>
#include <memory>
#include <vector>
#include <set>
#include "thread.h"
#include "util.h"

namespace sylar {

class TimerManager;

class Timer : public std::enable_shared_from_this<Timer> {
friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;
    bool cancle();
    bool refresh();
    bool reset(uint64_t ms, bool from_now);
private:
    // Timer对象不能自己创建，必须通过TimerManager来创建，所以我们给它设为私有
    Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager);   
    Timer(uint64_t next);
private:
    bool m_recurring = false;   // 是否循环计时器   循环计时：当前时间 + 定时时间
    uint64_t m_ms = 0;          // 执行周期
    uint64_t m_next = 0;        // 精确的执行时间
    std::function<void()> m_cb;
    TimerManager *m_manager = nullptr;
private:
    struct Comparator {
        bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
    };
};

class TimerManager {
friend class Timer;
public:
    typedef RWMutex RWMutexType;

    TimerManager();
    virtual ~TimerManager();

    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false);
    uint64_t getNextTimer();    // 获取下一个定时器的执行时间
    void listExpiredCb(std::vector<std::function<void()>> &cbs);  // 触发定时器后，返回那些已经超时的需要执行的cb
protected:  // 要与IO Event做交互
    virtual void onTimerInsertAtFront() = 0;
    void addTimer(Timer::ptr val, RWMutex::WriteLock &lock);
    bool hasTimer();
private:
    bool detectClockRollover(uint64_t now_ms);  // 用于检测服务器是否调过时间
private:
    RWMutexType m_mutex;
    std::set<Timer::ptr, Timer::Comparator> m_timers;   // 可以考虑用堆或者红黑树？(网友弹幕)
    bool m_tickled = false;
    uint64_t m_preTime;   // 服务器没有调过的上一个时间
};

}

#endif