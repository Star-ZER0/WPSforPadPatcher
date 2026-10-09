#include "native_hook.h"
#include <jni.h>

extern "C" JNIEXPORT jboolean JNICALL
Java_cc_star0_xposed_wpsforpadpatcher_NativeBridge_isFrameworkReady(JNIEnv*, jclass) {
    return wpspatch::framework_ready() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_cc_star0_xposed_wpsforpadpatcher_NativeBridge_isHookInstalled(JNIEnv*, jclass) {
    return wpspatch::hook_installed() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_cc_star0_xposed_wpsforpadpatcher_NativeBridge_probeAlreadyLoaded(JNIEnv*, jclass) {
    wpspatch::probe_already_loaded();
}
