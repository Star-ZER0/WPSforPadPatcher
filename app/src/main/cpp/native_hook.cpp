#include "native_hook.h"
#include "native_api.h"
#include "logging.h"

#include <atomic>
#include <cstring>
#include <dlfcn.h>
#include <limits.h>
#include <link.h>

namespace {
std::atomic<HookFunType> hook_function{nullptr};
enum class HookState { waiting, installing, installed };
std::atomic<HookState> hook_state{HookState::waiting};
std::atomic_flag first_call = ATOMIC_FLAG_INIT;
void* original_function = nullptr;

bool is_target_library(const char* path) {
    if (path == nullptr) return false;
    const char* slash = std::strrchr(path, '/');
    return std::strcmp(slash == nullptr ? path : slash + 1, wpspatch::kLibrary) == 0;
}

// A non-static C++ member function receives 'this' in x0 on AArch64.
// Its bool return value is carried in w0. Never call the original implementation.
bool always_valid([[maybe_unused]] void* application) {
    if (!first_call.test_and_set(std::memory_order_relaxed)) {
        PATCH_INFO("checkDeviceValidity intercepted -> true");
    }
    return true;
}

void try_install(void* handle) {
    const auto hook = hook_function.load(std::memory_order_acquire);
    if (handle == nullptr || hook == nullptr ||
        hook_state.load(std::memory_order_acquire) != HookState::waiting) return;

    // dlsym(handle) also searches dependencies. Android may load libwpsoffice.so
    // while libkprometheus.so is only observed as one of its DT_NEEDED entries.
    dlerror();
    void* target = dlsym(handle, wpspatch::kSymbol);
    const char* error = dlerror();
    if (error != nullptr || target == nullptr) return;

    Dl_info owner{};
    if (dladdr(target, &owner) == 0 || !is_target_library(owner.dli_fname)) return;

    HookState expected = HookState::waiting;
    if (!hook_state.compare_exchange_strong(expected, HookState::installing,
                                            std::memory_order_acq_rel)) return;

    const int result = hook(target, reinterpret_cast<void*>(always_valid), &original_function);
    if (result == 0) {
        hook_state.store(HookState::installed, std::memory_order_release);
        PATCH_INFO("Native hook installed: %s!%s at %p", owner.dli_fname, wpspatch::kSymbol, target);
    } else {
        hook_state.store(HookState::waiting, std::memory_order_release);
        PATCH_ERROR("Native hook failed (%d): %s; a later callback may retry", result, wpspatch::kSymbol);
    }
}

void on_library_loaded(const char* name, void* handle) {
    if (handle == nullptr) return;
    try_install(handle);
    if (is_target_library(name)
        && hook_state.load(std::memory_order_acquire) == HookState::waiting) {
        PATCH_ERROR("libkprometheus.so observed but hook not installed; check symbol/API compatibility");
    }
}

struct LoadedLibrary { char path[PATH_MAX]{}; };

int find_loaded_library(dl_phdr_info* info, [[maybe_unused]] size_t size, void* data) {
    if (!is_target_library(info->dlpi_name)) return 0;
    const auto length = std::strlen(info->dlpi_name);
    auto* found = static_cast<LoadedLibrary*>(data);
    if (length >= sizeof(found->path)) return 0;
    std::memcpy(found->path, info->dlpi_name, length + 1);
    return 1;
}
}

namespace wpspatch {
bool framework_ready() {
    return hook_function.load(std::memory_order_acquire) != nullptr;
}

bool hook_installed() {
    return hook_state.load(std::memory_order_acquire) == HookState::installed;
}

void probe_already_loaded() {
    if (!framework_ready() || hook_installed()) return;
    LoadedLibrary found;
    dl_iterate_phdr(find_loaded_library, &found);
    if (found.path[0] == '\0') return;

    // Outside dl_iterate_phdr's linker traversal; NOLOAD never initializes WPS early.
    void* handle = dlopen(found.path, RTLD_NOW | RTLD_NOLOAD);
    if (handle == nullptr) {
        PATCH_ERROR("Already loaded WPS library is not visible in this linker namespace");
        return;
    }
    try_install(handle);
    dlclose(handle);
}
}

extern "C" [[gnu::visibility("default")]] [[gnu::used]]
NativeOnModuleLoaded native_init(const NativeAPIEntries* entries) {
    if (entries == nullptr || entries->version < 1 || entries->hook_func == nullptr) {
        PATCH_ERROR("Invalid LSPosed Native Hook API entries");
        return nullptr;
    }
    // Copy the function pointer; the framework owns the entries object.
    hook_function.store(entries->hook_func, std::memory_order_release);
    PATCH_INFO("LSPosed Native Hook API initialized (version %u)", entries->version);
    return on_library_loaded;
}
