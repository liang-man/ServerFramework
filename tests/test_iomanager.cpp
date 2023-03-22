#include "sylar/sylar.h"
#include "sylar/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_fiber()
{
    SYLAR_LOG_INFO(g_logger) << "test_fiber";
}

void test1()
{
    sylar::IOManager iom;
    iom.schedule(&test_fiber); 
}

sylar::Timer::ptr s_timer;
void test_timer() 
{
    sylar::IOManager iom(2);
    s_timer = iom.addTimer(1000, []() {
        static int i = 0;
        SYLAR_LOG_INFO(g_logger) << "hello timer i=" << i;
        if (++i == 3) {
            // s_timer->cancle();
            s_timer->reset(2000, true);
        }
    }, true);
}

int main(int argc, char **argv)
{
    test_timer();
    
    return 0;
}