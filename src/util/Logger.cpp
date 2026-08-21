#include "drz/util/Logger.hpp"

#include <SDL3/SDL_log.h>
#include <vector>

namespace drz::logger
{
namespace
{ // anonymous for linkage only in this translation unit
std::vector<std::string> log_buffer;
}

void log(const std::string& message)
{
    log_buffer.push_back(message);
}

void flush_log()
{
    while (!log_buffer.empty())
    {
        // TODO: Log into another stream if we want to log into a file or something
        SDL_Log("%s", log_buffer.front().c_str()); // flush to SDL log
        log_buffer.clear();
    }
}
} // namespace drz::logger
