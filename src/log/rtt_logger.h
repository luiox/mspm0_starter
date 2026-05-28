/**
 * @file rtt_logger.h
 * @author canrad (1517807724@qq.com)
 * @brief 基于SEGGER RTT的轻量级日志库，提供基础的日志功能和格式化工具
 * 使用方法：在程序一开始要先调用 rlog_init() 进行初始化
 * 然后使用 log_error(), log_warn(), log_info(), log_debug() 等宏进行日志输出
 * log_raw() 可用于输出不带任何格式的原始字符串
 * 注意：针对于浮点数的格式化，f32_to_str()速度比较快，但是精度有限，
 * 而且不能处理很大的浮点数，因为截断法转字符串会溢出
 * @version 0.1
 * @date 2026-02-23
 *
 * @copyright Copyright (c) 2026
 *
 */
#ifndef RTT_LOGGER_H
#define RTT_LOGGER_H

#include "../util/datatype.h"
#include "SEGGER_RTT.h"
#include "format.h"

#ifdef __cplusplus
extern "C" {
#endif

// 配置项：可以在编译时通过定义以下宏来调整日志行为

#ifndef RTT_BUFFER_INDEX
#    define RTT_BUFFER_INDEX 0
#endif

#ifndef RTT_TRACE_BUFFER_INDEX
#    define RTT_TRACE_BUFFER_INDEX 1
#endif

#ifndef LOG_MODULE_NAME
#    define LOG_MODULE_NAME ""
#endif

#ifndef RLOG_ENABLE_TAG
#    define RLOG_ENABLE_TAG 1
#endif

#ifndef RLOG_DEBUG_SHOW_FILE_LINE
#    define RLOG_DEBUG_SHOW_FILE_LINE 1
#endif

#ifndef RLOG_LEVEL_BRIEF
#    define RLOG_LEVEL_BRIEF 0
#endif

#ifndef RLOG_ENABLE_COLOR
#    define RLOG_ENABLE_COLOR 1
#endif

///////////////////////////////////////////////////////////////////////////////

/* log levels */
#define RLOG_LEVEL_RAW 0
#define RLOG_LEVEL_ERROR 1
#define RLOG_LEVEL_WARN 2
#define RLOG_LEVEL_INFO 3
#define RLOG_LEVEL_DEBUG 4

#if RLOG_LEVEL_BRIEF
#    define _RLOG_LVL_STR_E "E"
#    define _RLOG_LVL_STR_W "W"
#    define _RLOG_LVL_STR_I "I"
#    define _RLOG_LVL_STR_D "D"
#else
#    define _RLOG_LVL_STR_E "ERROR"
#    define _RLOG_LVL_STR_W "WARN"
#    define _RLOG_LVL_STR_I "INFO"
#    define _RLOG_LVL_STR_D "DEBUG"
#endif

/* color config */
#if RLOG_ENABLE_COLOR
#    define _RLOG_COLOR_E RTT_CTRL_TEXT_BRIGHT_RED
#    define _RLOG_COLOR_W RTT_CTRL_TEXT_BRIGHT_YELLOW
#    define _RLOG_COLOR_I RTT_CTRL_TEXT_BRIGHT_GREEN
#    define _RLOG_COLOR_D RTT_CTRL_TEXT_BRIGHT_CYAN
#    define _RLOG_COLOR_RST RTT_CTRL_RESET
#else
#    define _RLOG_COLOR_E ""
#    define _RLOG_COLOR_W ""
#    define _RLOG_COLOR_I ""
#    define _RLOG_COLOR_D ""
#    define _RLOG_COLOR_RST ""
#endif

/* stringification helpers */
#define _RLOG_STRINGIFY(x) #x
#define _RLOG_TOSTRING(x) _RLOG_STRINGIFY(x)

#define _rlog_printf(fmt, ...) SEGGER_RTT_printf(RTT_BUFFER_INDEX, fmt, ##__VA_ARGS__)

// ============================================================
// 运行时 tag 栈机制
// ============================================================
// 推荐使用 push/pop 模式，支持嵌套调用时自动恢复调用者 tag：
//   rlog_push_tag("MyContext");
//   // ... log 输出显示 tag="MyContext"
//   // ... 子函数也可推入/弹出自己的 tag
//   rlog_pop_tag();  // 恢复之前 tag
//
// 栈为空时回退到编译期 LOG_MODULE_NAME。栈深度固定为 16。
// ============================================================
#define RLOG_TAG_STACK_DEPTH 16

// 推入新 tag（覆盖当前生效的 tag）
void rlog_push_tag(const char* tag);
// 弹出顶部 tag，恢复上一个
void rlog_pop_tag(void);
// 获取当前生效的 tag（栈非空返回栈顶，否则返回 NULL）
const char* rlog_get_effective_tag(void);

// 获取生效的 tag：运行时非空则优先使用，否则回退到编译期 LOG_MODULE_NAME
#if RLOG_ENABLE_TAG
#    define _RLOG_EFFECTIVE_TAG (rlog_get_effective_tag() ? rlog_get_effective_tag() : LOG_MODULE_NAME)
#    define _RLOG_TAG_NONEMPTY  1
#else
#    define _RLOG_EFFECTIVE_TAG ""
#    define _RLOG_TAG_NONEMPTY  0
#endif

/* core formatting macros */
// 当 _RLOG_EFFECTIVE_TAG 非空时，输出 "[level][tag] msg"；否则输出 "[level] msg"
#define _RLOG_CORE(level_str, level_color, fmt, ...) \
    do { \
        const char* _rlog_tag_ = _RLOG_EFFECTIVE_TAG; \
        if (_rlog_tag_[0] != '\0') { \
            _rlog_printf(level_color "[%s][%s] " fmt _RLOG_COLOR_RST "\n", \
                         level_str, _rlog_tag_, ##__VA_ARGS__); \
        } else { \
            _rlog_printf(level_color "[%s] " fmt _RLOG_COLOR_RST "\n", \
                         level_str, ##__VA_ARGS__); \
        } \
    } while(0)

#if RLOG_DEBUG_SHOW_FILE_LINE
#    define _RLOG_CORE_DEBUG(fmt, ...) \
        do { \
            const char* _rlog_tag_ = _RLOG_EFFECTIVE_TAG; \
            if (_rlog_tag_[0] != '\0') { \
                _rlog_printf(_RLOG_COLOR_D "[%s][%s][%s:%d] " fmt _RLOG_COLOR_RST "\n", \
                             _RLOG_LVL_STR_D, _rlog_tag_, __FILE__, __LINE__, ##__VA_ARGS__); \
            } else { \
                _rlog_printf(_RLOG_COLOR_D "[%s][%s:%d] " fmt _RLOG_COLOR_RST "\n", \
                             _RLOG_LVL_STR_D, __FILE__, __LINE__, ##__VA_ARGS__); \
            } \
        } while(0)
#else
#    define _RLOG_CORE_DEBUG(fmt, ...) _RLOG_CORE(_RLOG_LVL_STR_D, _RLOG_COLOR_D, fmt, ##__VA_ARGS__)
#endif

#define log_raw(fmt, ...) _rlog_printf(fmt, ##__VA_ARGS__)

#define log_error(fmt, ...) _RLOG_CORE(_RLOG_LVL_STR_E, _RLOG_COLOR_E, fmt, ##__VA_ARGS__)

#define log_warn(fmt, ...) _RLOG_CORE(_RLOG_LVL_STR_W, _RLOG_COLOR_W, fmt, ##__VA_ARGS__)

#define log_info(fmt, ...) _RLOG_CORE(_RLOG_LVL_STR_I, _RLOG_COLOR_I, fmt, ##__VA_ARGS__)

#define log_debug(fmt, ...) _RLOG_CORE_DEBUG(fmt, ##__VA_ARGS__)

// 定义标准的初始化接口
#define log_init() rlog_init()

// 初始化
static inline void rlog_init(void)
{
    SEGGER_RTT_Init();
}

#define _rtrace_printf(fmt, ...) SEGGER_RTT_printf(RTT_TRACE_BUFFER_INDEX, fmt, ##__VA_ARGS__)

#define _RTRACE_CORE(fmt, ...) \
    do { \
        _rtrace_printf(fmt, ##__VA_ARGS__); \
    } while(0)

#define trace_raw(fmt, ...) _RTRACE_CORE(fmt, ##__VA_ARGS__)


#ifdef __cplusplus
}
#endif

#endif   // LIBCA_EM_LOG_RTT_LOGGER_H
