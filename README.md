# WPS for Pad Patcher

使用 **libxposed API 102.0.0** 和 **LSPosed Native Hook** 的 Android 模块。

| 项目 | 值 |
| --- | --- |
| 模块包名 | `cc.star0.xposed.wpsforpadpatcher` |
| 固定作用域 | `cn.wps.moffice_hd` |
| 原生架构 | `arm64-v8a` |
| 最低 Android 版本 | Android 9 / API 28（同时受所用框架支持范围限制） |
| 已分析的 WPS 样本 | WPS for Pad 12.1.5.29016 |

## 实际行为

1. 尽早将目标进程中真实的 `android.os.Build.MANUFACTURER` 静态字段设为 `xiaomi`。WPS 通过 JNI 读取该字段时也会得到这个值。
2. 通过框架提供的 `NativeAPIEntries.hook_func`，将 `libkprometheus.so` 导出的 `_ZN17KPromeApplication19checkDeviceValidityEv` 替换为返回 `true` 的函数。

不修改 `BRAND`、`MODEL`、`Build.VERSION.RELEASE`、屏幕参数或系统属性。不修改 `libksolite.so`、`libkrt.so`，不替换 WPS APK 中的任何文件，也不使用固定内存偏移。

厂商模拟用于满足先前分析的 `PadSyncBindGate` / `PadOemBindGate`；Native Hook 用于跳过 `checkDeviceValidity()` 中的厂商、Android 版本和平板／多屏组合判断。模块不包含 WPS 本体。

## 安装使用

1. 使用同时支持 **modern libxposed API 102** 和 **Native Hook** 的 LSPosed 版本。仅支持旧 API 的框架无法加载本模块。
2. 安装模块 APK，在 LSPosed 中启用；作用域固定为 **WPS for Pad (`cn.wps.moffice_hd`)**。
3. 强制停止 WPS 后重新启动，确保其相关进程重新创建。这个模块没有桌面设置页面。
4. 在 LSPosed 日志或 logcat 中筛选 `WpsForPadPatcher`。成功路径应出现：

```text
Build.MANUFACTURER=xiaomi
LSPosed Native Hook API initialized (version ...)
Native hook installed: ...libkprometheus.so!...checkDeviceValidityEv...
checkDeviceValidity intercepted -> true
```

`waiting for WPS library` 表示已注册回调，正在等待目标库加载。辅助进程如果不加载这个库，可以一直处于等待状态。

修改、停用或更新模块后需要重启 WPS；模块明确拒绝 API 102 的热重载，以免旧原生回调／替换函数被卸载。无需重新签名 WPS。

## 构建

可用 Android Studio 打开本目录，或使用 JDK 17/21：

```sh
sdkmanager 'platforms;android-35' 'build-tools;35.0.0' 'ndk;27.2.12479018' 'cmake;3.22.1'
# 在 local.properties 写入 sdk.dir=/你的/Android/SDK/路径
./gradlew :app:assembleDebug
```

Windows 使用 `gradlew.bat :app:assembleDebug`。

调试 APK：`app/build/outputs/apk/debug/app-debug.apk`。调试构建会自动签名，可供本地安装测试；长期发布应使用自己的固定签名密钥。`assembleRelease` 默认生成未签名 APK。

项目固定 AGP 8.9.3、Gradle 8.13、NDK 27.2.12479018 和 CMake 3.22.1。Gradle Wrapper 带官方分发包 SHA-256 校验；`io.github.libxposed:api:102.0.0` 仅为 `compileOnly`，不会打包到模块中。

## 实现位置

- `app/src/main/java/cc/star0/xposed/wpsforpadpatcher/WpsModule.java`：API 102 生命周期、作用域过滤、厂商字段设置和原生库加载。
- `app/src/main/cpp/native_hook.cpp`：库加载回调、导出符号定位、函数所属库验证、幂等 Hook 及已加载库探测。
- `app/src/main/cpp/jni_bridge.cpp`：向 Java 层报告框架与 Hook 状态。
- `app/src/main/resources/META-INF/xposed/`：modern 模块入口、原生入口、固定作用域及 API 版本配置。

原生回调也检查加载句柄的依赖库，因此 Android 只回调 `libwpsoffice.so` 时也能定位其依赖 `libkprometheus.so` 的目标符号。`dladdr` 会校验符号确实来自 `libkprometheus.so`，避免误 Hook 其他库中碰巧同名的符号。已加载库探测使用 `RTLD_NOLOAD`，不会提前加载 WPS 库。

主进程与 `cn.wps.moffice_hd:*` 进程在 `onModuleLoaded()` 初始化；WPS 声明的其他进程名通过首个包回调确认归属后初始化。额外加载进来的其他应用包不会扩大作用域。

## 验证

已完成 Debug APK 构建、签名校验和 16 KB 对齐检查。尚未在 Android / LSPosed 真机上运行验证；模块的 16 KB 对齐也不代表 WPS 本体或框架支持 16 KB 页大小设备。

真机需要确认：冷启动、文字／表格／演示／PDF 的打开、编辑和保存；尤其是进入文字模块后，厂商复核是否仍通过。

若出现 `Native Hook API unavailable`，说明加载了模块 SO，但框架没有调用其 `native_init`；检查框架对 modern 原生入口的支持。如果目标库已加载但没有成功日志，查看符号、命名空间或 Hook 后端错误日志。不要将“APK 构建成功”视为“所有设备适配成功”。

## 接口依据

- [libxposed API 102 官方源码包](https://repo.maven.apache.org/maven2/io/github/libxposed/api/102.0.0/api-102.0.0-sources.jar)：生命周期、`META-INF/xposed/native_init.list`、`scope.list` 和模块配置。
- [LSPosed Native Hook ABI](https://github.com/LSPosed/LSPosed/wiki/Native-Hook)：`native_init`、加载回调和 `hook_func`。
