#include "timer.h"

namespace sylar {

bool Timer::Comparator::operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const 
{
    if (!lhs && !rhs) {
        return false;
    } 
    if (!lhs) {
        return true;
    }
    if (!rhs) {
        return false;
    }
    if (lhs->m_next < rhs->m_next) {
        return true;
    }
    if (rhs->m_next < lhs->m_next) {
        return false;
    }
    return lhs.get() < rhs.get();
}

Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager)
    : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager)
{
    m_next = sylar::GetCurrentMS() + m_ms;
}

Timer::Timer(uint64_t next) : m_next(next)
{

}

bool Timer::cancle()
{
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (m_cb) {
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());   // 找到这个定时器
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

// 重置时间
bool Timer::refresh()
{
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());    // 找到这个定时器
    if (it == m_manager->m_timers.end()) {
        return false;
    }
    // 为什么要先删除再添加回去呢？因为operator是基于next来做的，如果改时间的话，会影响比较位置
    // 重置时间一定是比当前时间要大，要往后走，不会走到它前面，所以不用addTimer去判断是否会加到m_timers前面
    m_manager->m_timers.erase(it);
    m_next = sylar::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

bool Timer::reset(uint64_t ms, bool from_now)
{
    if (ms == m_ms && !from_now) {   // 立马强制改时间
        return true;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) {
        return false;
    }

    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if (from_now) {
        start = sylar::GetCurrentMS();
    } else {
        start = m_next - m_ms;
    }
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->addTimer(shared_from_this(), lock);
    return true;
}

TimerManager::TimerManager() {
    m_preTime = sylar::GetCurrentMS();
}

TimerManager::~TimerManager()
{

}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
{
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(timer, lock);
    return timer;
}

// weak_ptr好处是不用引用计数加1，但又可以知道我们所指向的那个指针是否已经释放了
static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
{
    // lock是返回一下这个weak_ptr的智能指针，如果释放就是空的，如果没释放就拿到了这个指针
    std::shared_ptr<void> tmp = weak_cond.lock();
    if (tmp) {
        cb();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring)
{
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

uint64_t TimerManager::getNextTimer()
{
    RWMutexType::ReadLock lock(m_mutex);   // 没有做修改，用读锁
    m_tickled = false;
    if (m_timers.empty()) {
        return ~0ull;   // 如果是空的，返回最大的
    }

    // 否则就拿到首个定时器
    const Timer::ptr &next = *m_timers.begin();
    uint64_t now_ms = sylar::GetCurrentMS();
    if (now_ms >= next->m_next) {    // 说明这个定时器需要执行了，但不知什么原因晚了，那就立马执行
        return 0;
    } else {
        return next->m_next - now_ms;   // 返回还要等待的时间
    }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs)
{
    uint64_t now_ms = sylar::GetCurrentMS();    // 获取当前时间
    std::vector<Timer::ptr> expired;    // 存放已经超时的timer
    {
        RWMutexType::ReadLock lock(m_mutex);
        if (m_timers.empty()) {    // 如果是空的，说明没有任何定时器要执行，就直接返回
            return;
        }
    }
    RWMutex::WriteLock lock(m_mutex);    // 要用写锁，因为当如果真的有超时时间时，需要修改m_timers的

    bool rollover = detectClockRollover(now_ms);
    if (!rollover && ((*m_timers.begin())->m_next > now_ms)) {
        return;
    }

    Timer::ptr now_timer(new Timer(now_ms));
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);    // 发现调过时间了，就直接重置
    while (it != m_timers.end() && (*it)->m_next == now_ms) {
        ++it;
    }
    expired.insert(expired.begin(), m_timers.begin(), it);   // 把超时的需要执行的定时器放进去
    m_timers.erase(m_timers.begin(), it);    // 在m_tiners里把这部分定时器删掉
    cbs.reserve(expired.size());

    for (auto &timer : expired) {
        cbs.push_back(timer->m_cb);
        if (timer->m_recurring) {      // 如果timer是循环定时器，那我们要重置它的时间，然后再把它重新加回到m_timers里
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        } else {
            timer->m_cb = nullptr;     // 设置为nullptr，是防止回调函数了用了智能指针，如果不置空的话，会使得引用计数不减1
        }
    }
}

// 每次添加timer前先判断是不是在前面
void TimerManager::addTimer(Timer::ptr timer, RWMutex::WriteLock &lock)
{
    auto it = m_timers.insert(timer).first;   // m_timers用的是set，set是有序的，insert时可以返回这个数据的位置
    bool at_front = (it == m_timers.begin()) && !m_tickled;  // 如果位于首部位置，说明是即将要执行的定时器，它就要去唤醒一下原来的定时器
    if (at_front) {
        m_tickled = true;
    }
    lock.unlock();

    if (at_front) {
        onTimerInsertAtFront();    // 因为如果原来定时器定时10s，现在这个定时器3s后执行，那显然不能让原来定时器10s后再醒，不然就超出现在这个定时器时间了
    }
}

bool TimerManager::hasTimer()
{
    RWMutex::ReadLock lock(m_mutex);
    return !m_timers.empty();     // 不是空的就说明有定时器
}

bool TimerManager::detectClockRollover(uint64_t now_ms)
{
    bool rollover = false;
    if (now_ms < m_preTime && now_ms < (m_preTime - 60 * 60 + 1000)) {   // 当传进来的时间比它小，还比它一个小时前的时间小，那肯定是有问题的
        rollover = true;
    }
    m_preTime = now_ms;
    return rollover;
}

}