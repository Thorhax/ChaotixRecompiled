// The log macros the Thorhax intro (platforms/switch/thorhax_intro) is written
// with -- the ReXGlue SDK's, which the ports it was made for use -- on this
// program's log. The intro's own files stay as they are in every port.
#pragma once
#include <format>

#include "runtime/log.h"

#define REXLOG_INFO(...) LOGI("intro", "%s", std::format(__VA_ARGS__).c_str())
#define REXLOG_WARN(...) LOGW("intro", "%s", std::format(__VA_ARGS__).c_str())
