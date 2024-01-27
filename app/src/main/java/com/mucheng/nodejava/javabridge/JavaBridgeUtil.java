package com.mucheng.nodejava.javabridge;

import android.util.Log;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.util.Arrays;

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

        return new Object[]{field.getType(), field.getType().getComponentType(), field.get(target)};
    }

    public static Object[] getConstructor(String className, Object[] arguments) throws ClassNotFoundException, InvocationTargetException, IllegalAccessException, InstantiationException {
        Class<?> clazz = getClassLoader().loadClass(className);
        Constructor<?>[] constructors = clazz.getDeclaredConstructors();
        final Object[] finalArguments = new Object[arguments.length];

        constructorLoop:
        for (Constructor<?> constructor : constructors) {
            if (arguments.length != constructors.length) {
                continue;
            }

            Class<?>[] constructorParameterTypes = constructor.getParameterTypes();

            for (int index = 0; index < arguments.length; index++) {
                Class<?> constructorParameterType = constructorParameterTypes[index];
                if (constructorParameterType.isPrimitive()) {
                    if (arguments[index] == null) {
                        continue constructorLoop;
                    }


                } else {
                    if (arguments[index] == null) {
                        continue;
                    }

                    Class<?> argumentClass = arguments[index].getClass();
                    if (constructorParameterType.isAssignableFrom(argumentClass)) {
                        finalArguments[index] = arguments[index];
                        continue;
                    } else {
                        continue constructorLoop;
                    }
                }
            }

            if (isUnsafeReflectionEnabled()) {
                constructor.setAccessible(true);
            }

            return new Object[]{clazz, clazz.getComponentType(), constructor.newInstance(finalArguments)};
        }
        return null;
    }

}
