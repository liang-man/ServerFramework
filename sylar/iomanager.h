#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__

#include <memory>
#include "scheduler.h"
#include "thread.h"
#include "fiber.h"

namespace sylar {

class IOManager : Scheduler {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event {
        NONE = 0x0,
        READ = 0x1,
        WRITE = 0x2
    };
private:
    struct FdContext {
        typedef Mutex MutexType;
        struct EventContext {
            Scheduler *scheduler;    // 待执行的scheduler
            Fiber::ptr fiber;        // 事件协程
            std::function<void()> cb;// 事件的回调函数
        };

        EventContext &getContext(Event event);
        void resetContext(EventContext &ctx);
        void triggerEvent(Event event);

        int fd;                // 事件关联的句柄
        EventContext read;     // 读事件
        EventContext write;    // 写事件
        Event events = NONE;   // 已经注册的事件
        MutexType mutex;
    };
public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string &name = "");
    ~IOManager();

    // 1 success, 0 retry -1 error
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);     // 删除事件
    bool cancleEvent(int fd, Event event);  // 取消事件，并把触发事件的条件取消掉
    bool cancleAllEvent(int fd);
    static IOManager *GetThis();
protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;

    void contextResize(size_t size);
private:
    int m_epfd = 0;   // epoll_fd
    int m_tickleFds[2];

    std::atomic<size_t> m_pendingEventCount = {0};
    RWMutexType m_mutex;
    std::vector<FdContext *> m_fdContexts;   // 每个句柄有个上下文
};

}

#endif