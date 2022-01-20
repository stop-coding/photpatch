#include <stdio.h>
#include <string.h>
#include<string>

#pragma once


enum elog_level{
	ELOG_LEVEL_E_FATAL = 0,
	ELOG_LEVEL_E_ERROR,
	ELOG_LEVEL_E_WARN,
	ELOG_LEVEL_E_INFO,
	ELOG_LEVEL_E_DEBUG,
	ELOG_LEVEL_E_TRACE,
	ELOG_LEVEL_E_MAX,
};

#define HP_ERR (-1)

void elog_call(enum elog_level level, const char *module, const char *file,unsigned line, const char *function, const char *fmt, ...);

#define ELOG_ERROR(format, arg...) \
	elog_call(ELOG_LEVEL_E_ERROR, "HOTPATCH", __FILE__, __LINE__, __FUNCTION__, format, ##arg);
#define ELOG_WARN(format, arg...) \
	elog_call(ELOG_LEVEL_E_WARN, "HOTPATCH", __FILE__, __LINE__, __FUNCTION__, format, ##arg);
#define ELOG_NOTICE(format, arg...) \
	elog_call(ELOG_LEVEL_E_INFO, "HOTPATCH", __FILE__, __LINE__, __FUNCTION__, format, ##arg);
#define ELOG_DEBUG(format, arg...)\
	elog_call(ELOG_LEVEL_E_DEBUG, "HOTPATCH", __FILE__, __LINE__, __FUNCTION__, format, ##arg);
#define ELOG_TRACE(format, arg...)\
	elog_call(ELOG_LEVEL_E_TRACE, "HOTPATCH", __FILE__, __LINE__, __FUNCTION__, format, ##arg);

namespace ns_patch{

class elog{
public:
	static void error(const std::string &log) {elog_call(ELOG_LEVEL_E_ERROR, "HOTPATCH", __FILE__, __LINE__, __FUNCTION__, log.c_str());}
	static void info(const std::string &log) {elog_call(ELOG_LEVEL_E_INFO, "HOTPATCH", __FILE__, __LINE__, __FUNCTION__, log.c_str());}
	static void trace(const std::string &log) {elog_call(ELOG_LEVEL_E_TRACE, "HOTPATCH", __FILE__, __LINE__, __FUNCTION__, log.c_str());}
};

}