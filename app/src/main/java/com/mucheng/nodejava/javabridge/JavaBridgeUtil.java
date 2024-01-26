package com.mucheng.nodejava.javabridge;

import java.lang.reflect.Array;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
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

    private static Field getField(Object self, String fieldName) throws NoSuchFieldException {
        Class<?> clazz = self.getClass();
        Field field;
        if (isUnsafeReflectionEnabled()) {
            field = clazz.getDeclaredField(fieldName);
        } else {
            field = clazz.getField(fieldName);
        }

        return field;
    }

    private static Object[] getMethod(String className, String methodName, Object[] params) throws ClassNotFoundException {
        Class<?> clazz = getClassLoader().loadClass(className);
        Method[] methods;
        if (isUnsafeReflectionEnabled()) {
            methods = clazz.getDeclaredMethods();
        } else {
            methods = clazz.getMethods();
        }

        Object[] finalParams = new Object[params.length];
        outLoop:
        for (Method method : methods) {
            if (!method.getName().equals(methodName) || method.getParameterTypes().length != params.length) {
                continue;
            }

            Class<?>[] parameterTypes = method.getParameterTypes();
            for (int index = 0; index < params.length; index++) {
                Class<?> parameterType = parameterTypes[index];
                Object param = params[index];

                if (parameterType.isPrimitive()) {
                    if (param == null) {
                        continue outLoop;
                    }

                    Class<?> parameterReferenceType = getReferenceClassByPrimitive(parameterType);
                    Class<?> inputParameterType = param.getClass();

                    if (parameterReferenceType == Double.class) {
                        if (inputParameterType == Double.class) {
                            finalParams[index] = param;
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Byte.class) {
                        if (inputParameterType == Double.class) {
                            finalParams[index] = ((Double) param).byteValue();
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Short.class) {
                        if (inputParameterType == Double.class) {
                            finalParams[index] = ((Double) param).shortValue();
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Integer.class) {
                        if (inputParameterType == Double.class) {
                            try {
                                finalParams[index] = ((Double) param).intValue();
                            } catch (NumberFormatException ignored) {
                                continue outLoop;
                            }
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Long.class) {
                        if (inputParameterType == Double.class) {
                            try {
                                finalParams[index] = ((Double) param).longValue();
                            } catch (NumberFormatException ignored) {
                                continue outLoop;
                            }
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Float.class) {
                        if (inputParameterType == Double.class) {
                            try {
                                finalParams[index] = ((Double) param).floatValue();
                            } catch (NumberFormatException ignored) {
                                continue outLoop;
                            }
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Character.class) {
                        if (inputParameterType == String.class && ((String) param).length() == 1) {
                            finalParams[index] = ((String) param).charAt(0);
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Boolean.class) {
                        if (inputParameterType == Boolean.class) {
                            finalParams[index] = param;
                        } else {
                            continue outLoop;
                        }
                    } else {
                        continue outLoop;
                    }

                } else {
                    if (param == null) {
                        finalParams[index] = null;
                        continue;
                    }

                    Class<?> inputParameterType = param.getClass();
                    if (parameterType == inputParameterType) {
                        finalParams[index] = param;
                    } else {
                        continue outLoop;
                    }

                }

            }

            return new Object[]{method, finalParams};
        }

        String arguments = Arrays.toString(params);
        throw new IllegalArgumentException("Can't find method " + clazz.getName() + "." + methodName + "(" + arguments.substring(1, arguments.length() - 1) + ").");
    }

    private static Object[] getMethod(Object self, String methodName, Object[] params) {
        Class<?> clazz = self.getClass();
        Method[] methods;
        if (isUnsafeReflectionEnabled()) {
            methods = clazz.getDeclaredMethods();
        } else {
            methods = clazz.getMethods();
        }

        Object[] finalParams = new Object[params.length];
        outLoop:
        for (Method method : methods) {
            if (!method.getName().equals(methodName) || method.getParameterTypes().length != params.length) {
                continue;
            }

            Class<?>[] parameterTypes = method.getParameterTypes();
            for (int index = 0; index < params.length; index++) {
                Class<?> parameterType = parameterTypes[index];
                Object param = params[index];

                if (parameterType.isPrimitive()) {
                    if (param == null) {
                        continue outLoop;
                    }

                    Class<?> parameterReferenceType = getReferenceClassByPrimitive(parameterType);
                    Class<?> inputParameterType = param.getClass();

                    if (parameterReferenceType == Double.class) {
                        if (inputParameterType == Double.class) {
                            finalParams[index] = param;
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Byte.class) {
                        if (inputParameterType == Double.class) {
                            finalParams[index] = ((Double) param).byteValue();
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Short.class) {
                        if (inputParameterType == Double.class) {
                            finalParams[index] = ((Double) param).shortValue();
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Integer.class) {
                        if (inputParameterType == Double.class) {
                            try {
                                finalParams[index] = ((Double) param).intValue();
                            } catch (NumberFormatException ignored) {
                                continue outLoop;
                            }
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Long.class) {
                        if (inputParameterType == Double.class) {
                            try {
                                finalParams[index] = ((Double) param).longValue();
                            } catch (NumberFormatException ignored) {
                                continue outLoop;
                            }
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Float.class) {
                        if (inputParameterType == Double.class) {
                            try {
                                finalParams[index] = ((Double) param).floatValue();
                            } catch (NumberFormatException ignored) {
                                continue outLoop;
                            }
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Character.class) {
                        if (inputParameterType == String.class && ((String) param).length() == 1) {
                            finalParams[index] = ((String) param).charAt(0);
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Boolean.class) {
                        if (inputParameterType == Boolean.class) {
                            finalParams[index] = param;
                        } else {
                            continue outLoop;
                        }
                    } else {
                        continue outLoop;
                    }

                } else {
                    if (param == null) {
                        finalParams[index] = null;
                        continue;
                    }

                    Class<?> inputParameterType = param.getClass();
                    if (parameterType == inputParameterType) {
                        finalParams[index] = param;
                    } else {
                        continue outLoop;
                    }

                }

            }

            return new Object[]{method, finalParams};
        }

        String arguments = Arrays.toString(params);
        throw new IllegalArgumentException("Can't find method " + clazz.getName() + "." + methodName + "(" + arguments.substring(1, arguments.length() - 1) + ").");
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

    private static Object newInstance(String className, Object[] params) throws ClassNotFoundException, InvocationTargetException, IllegalAccessException, InstantiationException {
        Class<?> clazz = getClassLoader().loadClass(className);
        Constructor<?>[] constructors;
        if (isUnsafeReflectionEnabled()) {
            constructors = clazz.getDeclaredConstructors();
        } else {
            constructors = clazz.getConstructors();
        }

        Object[] finalParams = new Object[params.length];
        outLoop:
        for (Constructor<?> constructor : constructors) {
            if (constructor.getParameterTypes().length != params.length) {
                continue;
            }


            Class<?>[] parameterTypes = constructor.getParameterTypes();
            for (int index = 0; index < params.length; index++) {
                Class<?> parameterType = parameterTypes[index];
                Object param = params[index];

                if (parameterType.isPrimitive()) {
                    if (param == null) {
                        continue outLoop;
                    }

                    Class<?> parameterReferenceType = getReferenceClassByPrimitive(parameterType);
                    Class<?> inputParameterType = param.getClass();

                    if (parameterReferenceType == Double.class) {
                        if (inputParameterType == Double.class) {
                            finalParams[index] = param;
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Byte.class) {
                        if (inputParameterType == Double.class) {
                            finalParams[index] = ((Double) param).byteValue();
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Short.class) {
                        if (inputParameterType == Double.class) {
                            finalParams[index] = ((Double) param).shortValue();
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Integer.class) {
                        if (inputParameterType == Double.class) {
                            try {
                                finalParams[index] = ((Double) param).intValue();
                            } catch (NumberFormatException ignored) {
                                continue outLoop;
                            }
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Long.class) {
                        if (inputParameterType == Double.class) {
                            try {
                                finalParams[index] = ((Double) param).longValue();
                            } catch (NumberFormatException ignored) {
                                continue outLoop;
                            }
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Float.class) {
                        if (inputParameterType == Double.class) {
                            try {
                                finalParams[index] = ((Double) param).floatValue();
                            } catch (NumberFormatException ignored) {
                                continue outLoop;
                            }
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Character.class) {
                        if (inputParameterType == String.class && ((String) param).length() == 1) {
                            finalParams[index] = ((String) param).charAt(0);
                        } else {
                            continue outLoop;
                        }
                    } else if (parameterReferenceType == Boolean.class) {
                        if (inputParameterType == Boolean.class) {
                            finalParams[index] = param;
                        } else {
                            continue outLoop;
                        }
                    } else {
                        continue outLoop;
                    }

                } else {
                    if (param == null) {
                        finalParams[index] = null;
                        continue;
                    }

                    Class<?> inputParameterType = param.getClass();
                    if (parameterType == inputParameterType) {
                        finalParams[index] = param;
                    } else {
                        continue outLoop;
                    }

                }

            }

            constructor.setAccessible(true);
            return constructor.newInstance(finalParams);
        }

        String arguments = Arrays.toString(params);
        throw new IllegalArgumentException("Java constructor for \"" + className + "\" with arguments \"" + arguments.substring(1, arguments.length() - 1) + "\" not found.");
    }

    private static Class<?> getReferenceClassByPrimitive(Class<?> primitive) {
        switch (primitive.getName()) {
            case "byte":
                return Byte.class;
            case "short":
                return Short.class;
            case "int":
                return Integer.class;
            case "long":
                return Long.class;
            case "float":
                return Float.class;
            case "char":
                return Character.class;
            case "boolean":
                return Boolean.class;
            default:
                return Double.class;
        }
    }

}
