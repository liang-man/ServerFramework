#include "hook.h"
#include <dlfcn.h>

#include "config.h"
#include "log.h"
#include "fiber.h"
#include "iomanager.h"
#include "macro.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
namespace sylar {

static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
    sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

// 因为要hook的函数比较多，而且hook的方式类似，所以我们用宏来做这个声明和定义
#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) 

// 要初始化hook函数
void hook_init() {
    static bool is_inited = false;
    if(is_inited) {
        return;
    }
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);   // 从系统库里取函数并赋值给这个函数变量
    HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();    // 我们要确保在程序起来之前就已经hook了，所以要定义一个全局的变量
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value){
                SYLAR_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                         << old_value << " to " << new_value;
                s_connect_timeout = new_value;
        });
    }
};

static _HookIniter s_hook_initer;   // 实例化一个对象

bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

}

// 声明函数指针类型
extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
    if(!sylar::t_hook_enable) {
        return sleep_f(seconds);    // 如果没hook，就返回原函数
    }

    // 我们自己的实现
    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();     // 取出当前协程
    sylar::IOManager* iom = sylar::IOManager::GetThis();   // 取出当前iomanager
    // 在当前iomanager中加个定时器
    // iom->addTimer(seconds * 1000, std::bind((void(sylar::Scheduler::*)     
    //         (sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule
    //         ,iom, fiber, -1));
    iom->addTimer(seconds * 1000, [iom, fiber](){
        iom->schedule(fiber);
    });
    sylar::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec) {
    if(!sylar::t_hook_enable) {
        return usleep_f(usec);
    }
    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    // iom->addTimer(usec / 1000, std::bind((void(sylar::Scheduler::*)
    //         (sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule
    //         ,iom, fiber, -1));
    iom->addTimer(usec / 1000, [iom, fiber](){
        iom->schedule(fiber);
    });
    sylar::Fiber::YieldToHold();
    return 0;
}

}