#pragma once

#include <android/log.h>
#define PATCH_INFO(...) __android_log_print(ANDROID_LOG_INFO, "WpsForPadPatcher", __VA_ARGS__)
#define PATCH_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, "WpsForPadPatcher", __VA_ARGS__)
