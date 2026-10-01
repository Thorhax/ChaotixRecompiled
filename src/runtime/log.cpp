#include "log.h"
#include <cstdio>

namespace chaotix {

static LogLevel g_level = LogLevel::Info;
static FILE* g_log_file = nullptr;
static bool g_log_file_owned = false;

void log_set_level(LogLevel lvl) { g_level = lvl; }
LogLevel log_get_level() { return g_level; }

bool log_open_file(const char* path) {
    log_close_file();
    g_log_file = std::fopen(path, "w");
    g_log_file_owned = (g_log_file != nullptr);
    if (g_log_file) {
        std::fprintf(g_log_file, "=== Chaotix Runtime Log Started ===\n");
        std::fflush(g_log_file);
    }
    return g_log_file != nullptr;
}

void log_close_file() {
    if (g_log_file && g_log_file_owned) {
        std::fclose(g_log_file);
    }
    g_log_file = nullptr;
    g_log_file_owned = false;
}

void log_set_file(FILE* f) {
    log_close_file();
    g_log_file = f;
    g_log_file_owned = false;
}

void log_msg(LogLevel lvl, const char* category, const char* fmt, ...) {
    if (lvl > g_level) return;
    static const char* names[] = {"E", "W", "I", "D", "T"};
    const char* lvl_char = (int(lvl) >= 0 && int(lvl) < 5) ? names[int(lvl)] : "?";

    std::fprintf(stderr, "[%s][%s] ", lvl_char, category);
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
    std::fputc('\n', stderr);

    if (g_log_file) {
        std::fprintf(g_log_file, "[%s][%s] ", lvl_char, category);
        va_start(ap, fmt);
        std::vfprintf(g_log_file, fmt, ap);
        va_end(ap);
        std::fputc('\n', g_log_file);
        std::fflush(g_log_file);
    }
}

} // namespace chaotix
