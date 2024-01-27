package com.mucheng.nodejava.javabridge;

import android.util.Log;

import java.lang.reflect.Field;

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

    public static Object[] getField(String className, String fieldName, Object target) throws ClassNotFoundException, NoSuchFieldException, IllegalAccessException {
        Class<?> clazz = getClassLoader().loadClass(className);
        Field field = clazz.getDeclaredField(fieldName);
        if (isUnsafeReflectionEnabled()) {
            field.setAccessible(true);
        }

        Log.e("Target", "" + field.get(target));

        return new Object[]{field.getType(), field.getType().getComponentType(), field.get(target)};
    }

}
