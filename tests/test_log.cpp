#include <iostream>
#include "../sylar/log.h"
#include "../sylar/util.h"
#include "../sylar/singleton.h"

int main(int argc, char **argv)
{
    sylar::Logger::ptr logger(new sylar::Logger);
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));

    sylar::FileLogAppender::ptr file_appender(new sylar::FileLogAppender("../log.txt"));
    sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(sylar::LogLevel::ERROR);    // 设置log.txt中可以放入的日志级别

    logger->addAppender(file_appender);

    // sylar::LogEvent::ptr event(new sylar::LogEvent(__FILE__, __LINE__, 0, sylar::GetThreadId(), sylar::GetFiberId(), time(0)));
    // logger->log(sylar::LogLevel::DEBUG, event);

    SYLAR_LOG_INFO(logger) << "test looger INFO";        // 因为上面设置了级别为ERROR，所以这条日志不可以被写入文件
    SYLAR_LOG_ERROR(logger) << "test looger ERROR";      // 因为上面设置了级别为ERROR，所以这条日志可以被写入文件

    SYLAR_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");   // 因为上面设置了级别为ERROR，所以这条日志可以被写入文件

    auto l = sylar::LoggerMgr::GetInstance()->getLogger("xx");   // 测试日志管理器类
    SYLAR_LOG_INFO(l) << "xxx";

    return 0;
}