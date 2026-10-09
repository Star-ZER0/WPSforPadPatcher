package cc.star0.xposed.wpsforpadpatcher;

final class NativeBridge {
    private NativeBridge() {}

    static native boolean isFrameworkReady();
    static native boolean isHookInstalled();
    static native void probeAlreadyLoaded();
}
