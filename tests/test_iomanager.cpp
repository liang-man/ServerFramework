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

int main(int argc, char **argv)
{
    test1();
    
    return 0;
}