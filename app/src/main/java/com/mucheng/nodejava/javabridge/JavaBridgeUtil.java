package com.mucheng.nodejava.javabridge;

/**
 * @noinspection unused
 */
public final class JavaBridgeUtil {

    private static boolean unsafeReflectionEnabled;

    private JavaBridgeUtil() {
    }

    private static ClassInfo findClassOrNull(String className) {
        ClassLoader classLoader = getClassLoader();
        try {
            return new ClassInfo(classLoader.loadClass(className));
        } catch (ClassNotFoundException e) {
            return null;
        }
    }

    private static ClassLoader getClassLoader() {
        return JavaBridgeUtil.class.getClassLoader();
    }

    public static void setUnsafeReflectionEnabled(boolean isEnabled) {
        unsafeReflectionEnabled = isEnabled;
    }

    public static boolean isUnsafeReflectionEnabled() {
        return unsafeReflectionEnabled;
    }

}
