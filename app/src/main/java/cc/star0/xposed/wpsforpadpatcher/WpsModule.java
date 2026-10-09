package cc.star0.xposed.wpsforpadpatcher;

import android.os.Build;
import android.util.Log;

import java.lang.reflect.Field;

import io.github.libxposed.api.XposedModule;

/** One module instance per injected WPS process. No legacy Xposed API is used. */
public final class WpsModule extends XposedModule {
    private static final String TAG = "WpsForPadPatcher";
    private static final String TARGET_PACKAGE = "cn.wps.moffice_hd";
    private boolean targetProcess;
    private boolean initialized;
    private String processName;

    @Override
    public void onModuleLoaded(ModuleLoadedParam param) {
        processName = param.getProcessName();
        if (param.isSystemServer()) {
            detach();
            return;
        }
        targetProcess = isTargetProcess(processName);

        // Earlier than Application/AppComponentFactory initialization, so WPS cannot
        // cache the original manufacturer before its native libraries are loaded.
        if (targetProcess) initialize();
    }

    @Override
    public void onPackageLoaded(PackageLoadedParam param) {
        onTargetPackage(param);
    }

    @Override
    public void onPackageReady(PackageReadyParam param) {
        onTargetPackage(param);
    }

    @Override
    public boolean onHotReloading(HotReloadingParam param) {
        // Native callbacks/replacement code must live for the entire target process.
        // Restart WPS after changing or disabling this module.
        return false;
    }

    private static boolean isTargetProcess(String name) {
        return TARGET_PACKAGE.equals(name)
                || (name != null && name.startsWith(TARGET_PACKAGE + ":"));
    }

    private void onTargetPackage(PackageLoadedParam param) {
        // WPS also declares a global meeting-service process name. Verify its first
        // package instead of spoofing arbitrary processes with a similar name.
        if (param.isFirstPackage() && !TARGET_PACKAGE.equals(param.getPackageName())) {
            if (!targetProcess) detach();
            return;
        }
        if (TARGET_PACKAGE.equals(param.getPackageName())
                && (targetProcess || param.isFirstPackage())) {
            targetProcess = true;
            initialize();
        }
    }

    private synchronized void initialize() {
        if (initialized) {
            return;
        }
        try {
            // Android's field is initialized at runtime, not a Java constant literal.
            // Changing the actual field also affects WPS's JNI GetStaticObjectField.
            Field manufacturer = Build.class.getDeclaredField("MANUFACTURER");
            manufacturer.setAccessible(true);
            manufacturer.set(null, "xiaomi");
            if (!"xiaomi".equals(manufacturer.get(null))) {
                throw new IllegalStateException("Build.MANUFACTURER did not change");
            }
            log(Log.INFO, TAG, "[" + processName + "] Build.MANUFACTURER=xiaomi");

            System.loadLibrary("wpsforpadpatcher");
            if (!NativeBridge.isFrameworkReady()) {
                log(Log.ERROR, TAG, "Native Hook API unavailable: native_init was not called. "
                        + "Use an LSPosed build supporting API 102 and Native Hook.");
                return;
            }
            NativeBridge.probeAlreadyLoaded();
            initialized = true;
            log(Log.INFO, TAG, "[" + processName + "] Native callback ready; "
                    + "checkDeviceValidity hook "
                    + (NativeBridge.isHookInstalled() ? "installed" : "waiting for WPS library"));
        } catch (ReflectiveOperationException | RuntimeException | LinkageError error) {
            log(Log.ERROR, TAG, "[" + processName + "] Module initialization failed", error);
        }
    }
}
