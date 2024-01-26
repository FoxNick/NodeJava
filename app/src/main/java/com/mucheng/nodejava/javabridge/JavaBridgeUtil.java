package com.mucheng.nodejava.javabridge;

import android.util.Log;

import java.lang.reflect.Array;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

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

    private static Class<?> classForName(String className) throws ClassNotFoundException {
        return getClassLoader().loadClass(className);
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

    private static Field getField(String className, String fieldName) throws ClassNotFoundException, NoSuchFieldException {
        Class<?> clazz = getClassLoader().loadClass(className);
        Field field;
        if (isUnsafeReflectionEnabled()) {
            field = clazz.getDeclaredField(fieldName);
        } else {
            field = clazz.getField(fieldName);
        }

        return field;
    }

    private static Method getMethod(String className, String methodName) throws ClassNotFoundException, NoSuchMethodException {
        Class<?> clazz = getClassLoader().loadClass(className);
        Method method;
        if (isUnsafeReflectionEnabled()) {
            method = clazz.getDeclaredMethod(methodName);
        } else {
            method = clazz.getMethod(methodName);
        }
        return method;
    }

    private static Object getArrayElement(Object array, int index) {
        return Array.get(array, index);
    }

    private static double asNumber(Object value) {
        if (value instanceof Byte) {
            return ((Byte) value).doubleValue();
        }
        if (value instanceof Short) {
            return ((Short) value).doubleValue();
        }
        if (value instanceof Integer) {
            return ((Integer) value).doubleValue();
        }
        if (value instanceof Long) {
            return ((Long) value).doubleValue();
        }
        if (value instanceof Float) {
            return ((Float) value).doubleValue();
        }
        return (Double) value;
    }

    private static String asString(Object value) {
        if (value instanceof Character) {
            return value.toString();
        }
        return (String) value;
    }

    private static boolean asBoolean(Object value) {
        return (Boolean) value;
    }

}
