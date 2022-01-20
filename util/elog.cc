#include <sys/syscall.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include "elog.h"


#define LOG_TIME_FMT "%04d/%02d/%02d-%02d:%02d:%02d.%05ld"
static pid_t gettid()
{
	return syscall(SYS_gettid);
}

void elog_call(enum elog_level level, const char *module, const char *file,unsigned line, const char *function, const char *fmt, ...)
{
	va_list			args;
	const char		*short_file;
	struct timeval	tv;
	struct tm		t;
	char			buf[2048];
	char			buf2[256];
	int			    length = 0;
	static const char* level_str[ELOG_LEVEL_E_MAX] = {
		"FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"
	};
	time_t time1;
    
	va_start(args, fmt);
	length = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	buf[length] = 0;

	gettimeofday(&tv, NULL);
	time1 = (time_t)tv.tv_sec;
	localtime_r(&time1, &t);

	short_file = strrchr(file, '/');
	short_file = (!short_file) ? file : short_file + 1;
	if(level <= ELOG_LEVEL_E_ERROR) {
		snprintf(buf2, sizeof(buf2), "%s:%u|%s", short_file, line, function);
	}else{
		snprintf(buf2, sizeof(buf2), "%s:%u", short_file, line);
	}

    fprintf(stderr, "[%-5s][%-5x][%-5s] [" LOG_TIME_FMT "] %-24s - %s\n", module, gettid(),
            level_str[level],
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
            t.tm_hour, t.tm_min, t.tm_sec, tv.tv_usec,
            buf2,
            buf);
}