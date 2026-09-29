/**
 * @file   LogMacros.h
 * @brief  printf 风格日志宏定义（幂等：可重复包含）
 * @author libafl
 * @date   2026-09
 */
#undef LOG_TRACE
#undef LOG_DEBUG
#undef LOG_INFO
#undef LOG_NOTICE
#undef LOG_WARN
#undef LOG_WARNING
#undef LOG_ERROR
#undef LOG_CRITICAL
#undef LOG_CRITICA
#undef LOG_ALERT
#undef LOG_EMERGENCY

#define LOG_TRACE(...) ::afl::log::logPrintf(spdlog::level::trace, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) ::afl::log::logPrintf(spdlog::level::debug, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) ::afl::log::logPrintf(spdlog::level::info, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_NOTICE(...) ::afl::log::logPrintf(spdlog::level::info, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) ::afl::log::logPrintf(spdlog::level::warn, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...) ::afl::log::logPrintf(spdlog::level::warn, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) ::afl::log::logPrintf(spdlog::level::err, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_CRITICAL(...)                                                                          \
    ::afl::log::logPrintf(spdlog::level::critical, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_CRITICA(...)                                                                           \
    ::afl::log::logPrintf(spdlog::level::critical, __FILE__, __LINE__, __VA_ARGS__) // 兼容旧拼写
#define LOG_ALERT(...)                                                                             \
    ::afl::log::logPrintf(spdlog::level::critical, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_EMERGENCY(...)                                                                         \
    ::afl::log::logPrintf(spdlog::level::critical, __FILE__, __LINE__, __VA_ARGS__)
