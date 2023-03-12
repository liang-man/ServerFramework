// 防止头文件被重复包含，也可以用once
#ifndef __STLAR_LOG_H_
#define __STLAR_LOG_H_

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include "util.h"
#include "singleton.h"

// 为了方便日志的使用
// 这里在宏内部用if语句构造了局部变量LogEventWrap，很巧妙的利用了局部变量的生命周期实现打印：LogEventWrap在if语句结束后会结束生命周期从而用其析构函数实现打印
#define SYLAR_LOG_LEVEL(logger, level) \
    if (logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
            __FILE__, __LINE__, 0, sylar::GetThreadId(), \
            sylar::GetFiberId(), time(0)))).getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if (logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
        __FILE__, __LINE__, 0, sylar::GetThreadId(), \
        sylar::GetFiberId(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)

#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)

// 防止与别人的项目重命名了，因为项目叫sylar，所以命名空间也叫sylar了
namespace sylar {

// 关于类的前置声明，看这篇收藏的网址https://blog.csdn.net/whahu1989/article/details/83627658
class Logger;

// 日志级别
class LogLevel {
public:
    enum Level {
        UNKNOW = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };

    static const char *ToString(LogLevel::Level level);
    static LogLevel::Level FromString(const std::string &str); 
};

// 每个日志生成出来的时候，把它定义成一个logevent，它所需要的属性、字段都放在里面
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level val, const char *file, int32_t m_line, uint32_t elapse,
            uint32_t thread_id, uint32_t fiber_id, uint64_t time);

    const char *getFile() const { return m_file; }
    int32_t getLine() const { return m_line; }
    uint32_t getElapse() const { return m_elapse; }
    uint32_t getThreadId() const { return m_threadId; }
    uint32_t getFiberId() const { return m_fiberId; }
    uint64_t getTime() const { return m_time; }
    std::string getContent() const { return m_ss.str(); }
    std::shared_ptr<Logger> getLogger() const { return m_logger; }
    LogLevel::Level getLevel() const { return m_level; }

    std::stringstream &getSS() { return m_ss; }
    void format(const char *fmt, ...);
    void format(const char *fmt, va_list al);
private:
    const char *m_file = nullptr;   // 文件名
    int32_t m_line = 0;             // 行号
    uint32_t m_elapse = 0;          // 程序启动开始到现在的毫秒数
    uint32_t m_threadId = 0;        // 线程号
    uint32_t m_fiberId = 0;         // 协程号
    uint64_t m_time;                // 时间戳
    std::stringstream m_ss;          // 消息

    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;
};

// 因为LogEvent用的是智能指针，直接用不合适
// 所以用LogEventWrap封装LogEvent，Wrap在析构的时候，然后再把这个event写到logger里面去
class LogEventWrap {
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    std::stringstream &getSS();

    LogEvent::ptr getEvent() const { return m_event; }
private:
    LogEvent::ptr m_event;
};

// 日志格式
class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string &pattern);

    // 把日志输出成我们设定的固定化格式
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    void init();
    bool isError() const { return m_isError; }
    const std::string getPattern() const { return m_pattern; } 
public:
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        // FormatItem(const std::string &fmt = "") {}
        virtual ~FormatItem() {}

        // 直接把它输出到流里，因为它会多个组合起来，这样可能性能会好一点
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };

private:
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_items;
    bool m_isError = false;
};

// 日志输出地
class LogAppender {
public:
    typedef std::shared_ptr<LogAppender> ptr;
    // 因为日志输出的地方有很多，所以把它定义成虚类. 如果不定义成虚析构函数，那么它的子类释放的时候，可能会出现一些释放的问题
    virtual ~LogAppender() {}

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

    virtual std::string toYamlString() = 0;
    
    void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
    LogFormatter::ptr getFormatter() const { return m_formatter; }

    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level val) { m_level = val; }
protected:
    LogLevel::Level m_level = LogLevel::DEBUG;    // 定义这个Appender主要针对哪个日志级别   定义为proteced，这样它的子类就可以使用到
    LogFormatter::ptr m_formatter;
};

// 日志器，定义日志类别
// 这里为什么使用enable_shared_from_this，见收藏的网址https://blog.csdn.net/caoshangpa/article/details/79392878
class Logger : public std::enable_shared_from_this<Logger> {
friend class LoggerManager;
public:
    typedef std::shared_ptr<Logger> ptr; 

    Logger(const std::string &name = "root");
    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppenders();
    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level val) { m_level = val; }

    const std::string &getName() const { return m_name; }

    void setFormatter(LogFormatter::ptr val);
    void setFormatter(const std::string &val);   // 这里必须是常量引用，而不是引用
    LogFormatter::ptr getFormatter();

    std::string toYamlString();
private:
    std::string m_name;        // 日志名称
    LogLevel::Level m_level;   // 定义日志器的日志级别
    std::list<LogAppender::ptr> m_appenders;  // Appender集合
    LogFormatter::ptr m_formatter;
    Logger::ptr m_root;        // 针对开发笔记77-82行的问题所做的改变
};

// 定义输出到控制台的Appender
class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;
};

// 定义输出到文件的Appender
class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string &filename);
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;

    bool reopen();   // 重新打开文件，打开成功返回true
private:
    std::string m_filename;
    std::ofstream m_filestream;
};

class LoggerManager {
public:
    LoggerManager();
    Logger::ptr getLogger(const std::string &name);   

    void init();
    Logger::ptr getRoot() const { return m_root; }
    std::string toYamlString();
private:
    std::map<std::string, Logger::ptr> m_loggers;
    Logger::ptr m_root;
};

typedef sylar::Singleton<LoggerManager> LoggerMgr;

}

#endif