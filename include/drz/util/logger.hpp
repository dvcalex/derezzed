#pragma once

#include <string>
#include <format>
#include <utility>

namespace drz::logger
{

void Log(const std::string& message);
void FlushLog();

template <typename... Args> void Logf(std::format_string<Args...> fmt, Args&&... args)
{
    Log(std::format(fmt, std::forward<Args>(args)...));
}

} // namespace drz::logger

#ifndef NDEBUG
#define DRZ_LOG(msg) ::drz::logger::Log(msg)
#define DRZ_LOGF(...) ::drz::logger::Logf(__VA_ARGS__)
#define DRZ_FLUSH_LOG() ::drz::logger::FlushLog()
#else
#define DRZ_LOG(msg) ((void)0)
#define DRZ_LOGF(...) ((void)0)
#define DRZ_FLUSH_LOG() ((void)0)
#endif
