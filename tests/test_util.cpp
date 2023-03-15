#include "sylar/sylar.h"
#include <assert.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_assert()
{
    // skip掉2层，这样打印出的栈就好看一点，整洁一点，没有额外的信息
    SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10);
    SYLAR_ASSERT2(0==1, "abcdefg xx");
}

int main(int argc, char **argv)
{
    test_assert();

    return 0;
}