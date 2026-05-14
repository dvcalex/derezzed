#include <drz/util/logger.hpp>
#include <SDL3/SDL_log.h>
#include <queue>

namespace drz::logger {
namespace { // anonymous for linkage only in this translation unit
std::queue<std::string> log_queue;
}

void log(const std::string& message) {
    log_queue.push(message);
}

void flush_log() {
    while (!log_queue.empty()) {
        // TODO: Log into another stream if we want to log into a file or something, but for now just log to SDL's logging system
        SDL_Log("%s", log_queue.front().c_str());
        log_queue.pop();
    }
}
} // namespace drz::logger
