#pragma once

namespace wpspatch {
inline constexpr char kLibrary[] = "libkprometheus.so";
inline constexpr char kSymbol[] = "_ZN17KPromeApplication19checkDeviceValidityEv";

bool framework_ready();
bool hook_installed();
void probe_already_loaded();
}
