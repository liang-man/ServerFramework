#include "util.h"
#include <execinfo.h>

#include "log.h"
#include "fiber.h"

namespace sylar {

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

pid_t GetThreadId()
{
    return syscall(SYS_gettid);
} 

uint32_t GetFiberId()
{
    return sylar::Fiber::GetFiberId();
}

// 在扔出一个exception的时候，除了应该包含错误，还应该包含栈，这样我们就知道错误是从哪个栈抛出来的，方便调试
void Backtrace(std::vector<std::string> &bt, int size, int skip)
{
    // 线程的栈一般都比较大，有个8M或10M，而我们协程需要轻量级别换，且从内存节省考虑也不会创建很多层栈
    void **array = (void **)malloc((sizeof (void *) * size));
    size_t s = ::backtrace(array, size);

    char **strings = backtrace_symbols(array, s);
    if (strings == NULL) {    // NULL与nullptr区别？
        SYLAR_LOG_INFO(g_logger) << "backtrace_symbols error";
        return;
    }

    for (size_t i = skip; i < s; ++i) {
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);
}

std::string BacktraceToString(int size, int skip, const std::string &prefix)
{
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for (size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}

}