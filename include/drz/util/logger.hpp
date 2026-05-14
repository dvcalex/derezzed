#pragma once

#include <string>
#include <format>
#include <utility>

namespace drz::logger {

void log(const std::string& message);
void flush_log();

template <typename... Args> void logf(std::format_string<Args...> fmt, Args&&... args) {
    log(std::format(fmt, std::forward<Args>(args)...));
}

} // namespace drz::logger

#ifndef NDEBUG
#define DRZ_LOG(msg) ::drz::logger::log(msg)
#define DRZ_LOGF(...) ::drz::logger::logf(__VA_ARGS__)
#define DRZ_FLUSH_LOG() ::drz::logger::flush_log()
#else
#define DRZ_LOG(msg) ((void)0)
#define DRZ_LOGF(...) ((void)0)
#define DRZ_FLUSH_LOG() ((void)0)
#endif
