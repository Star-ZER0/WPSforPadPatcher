#pragma once

#include <cstdint>

// LSPosed Native Hook ABI:
// https://github.com/LSPosed/LSPosed/wiki/Native-Hook
using HookFunType = int (*)(void* function, void* replacement, void** backup);
using UnhookFunType = int (*)(void* function);
using NativeOnModuleLoaded = void (*)(const char* name, void* handle);

struct NativeAPIEntries {
    uint32_t version;
    HookFunType hook_func;
    UnhookFunType unhook_func;
};

extern "C" [[gnu::visibility("default")]] [[gnu::used]]
NativeOnModuleLoaded native_init(const NativeAPIEntries* entries);
