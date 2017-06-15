#ifndef LOG
#define LOG

#include <string>
#include <stdio.h>

#if defined(_LINUX_PLATFORM_)
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <syslog.h>
#endif

#if defined(_WIN32_PLATFROM_)
#include <windows.h>
#endif

#include <fcntl.h>
#include <time.h>
#include <utility>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <map>

class Log {
public:
    typedef std::map<int, std::string> MapLogLevelStr;
    enum LogLevel
    {
        E_LOG_DEBUG = 0,
        E_LOG_INFO  = 1,
        E_LOG_WARN  = 2,
        E_LOG_EROR  = 3,
        E_LOG_FATAL = 4,
    };

    Log()
    {
        level_ = E_LOG_DEBUG;
        mLogLv[E_LOG_DEBUG] = "DBUG";
        mLogLv[E_LOG_INFO] = "INFO";
        mLogLv[E_LOG_WARN] = "WARN";
        mLogLv[E_LOG_EROR] = "EROR";
        mLogLv[E_LOG_FATAL] = "FATL";
    }
    ~Log(){}

    static Log& getLog()
    {
        static Log l;
        return l;
    }

    void setLogLevel( int level ) { level_ = level; }

    void print(int level, int line, const char* file, const char* function, const char* fmt... )
    {
        char buffer[4 * 1024] = {0};
        char *begin = buffer;
        char *end = buffer + sizeof(buffer);

        va_list args;
        time_t seconds;
#if defined(_LINUX_PLATFORM_)
        struct timeval now;
        gettimeofday(&now, NULL);
        seconds = now.tv_sec;
#endif
#if defined(_WIN32_PLATFROM_)
        (void) time(&seconds);
#endif
        struct tm t;
        localtime_r(&seconds, &t);

        begin += snprintf(begin, end - begin,
                "%04d/%02d/%02d-%02d:%02d:%02d [%s] [%10s] %s:%d ",
                t.tm_year + 1900,
                t.tm_mon + 1,
                t.tm_mday,
                t.tm_hour,
                t.tm_min,
                t.tm_sec,
                mLogLv[level].c_str(),
                function,
                file,
                line);

        va_start(args, fmt);
        begin += vsnprintf(begin, end-begin, fmt, args);
        va_end(args);
        if ( level_ <= level ) printf("%s\n", buffer);
        fflush(stdout);
    }
private:
    int level_;
    MapLogLevelStr mLogLv;
};

#define klog(level, ...) \
    do {\
        Log::getLog().print(level, __LINE__, __FILE__, __func__, __VA_ARGS__);\
    }while(0)

#define klog_debg(...) \
        klog(Log::LogLevel::E_LOG_DEBUG, __VA_ARGS__)
#define klog_info(...) \
        klog(Log::LogLevel::E_LOG_INFO, __VA_ARGS__)
#define klog_error(...) \
        klog(Log::LogLevel::E_LOG_EROR, __VA_ARGS__)
#define klog_warn(...) \
        klog(Log::LogLevel::E_LOG_WARN, __VA_ARGS__)
#define klog_fatal(...) \
        klog(Log::LogLevel::E_LOG_FATAL, __VA_ARGS__)

#endif // LOG

