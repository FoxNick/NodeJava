package com.mucheng.nodejava.javabridge;

import android.util.Log;

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
      try {
        field.setAccessible(true);
      } catch (SecurityException ignored) {
      }
    }

    return new Object[]{field.getType(), field.getType().getComponentType(), field.get(target)};
  }

  public static Object[] callMethod(String className, String methodName, Object[] arguments, Object target) throws ClassNotFoundException, InvocationTargetException, IllegalAccessException {
    Class<?> clazz = getClassLoader().loadClass(className);
    Method[] methods = clazz.getDeclaredMethods();
    final Object[] finalArguments = new Object[arguments.length];

    methodLoop:
    for (Method method : methods) {
      Class<?>[] methodParameterTypes = method.getParameterTypes();
      if (!method.getName().equals(methodName)) {
        continue;
      }
      if (arguments.length != methodParameterTypes.length) {
        continue;
      }

      for (int index = 0; index < arguments.length; index++) {
        Class<?> methodParameterType = methodParameterTypes[index];
        if (methodParameterType.isPrimitive()) {
          if (arguments[index] == null) {
            continue methodLoop;
          }

          String constParameterType = methodParameterType.getName();
          String argumentType = getReferencePrimitiveType(arguments[index].getClass());
          if (argumentType == null) {
            continue methodLoop;
          }

          if (constParameterType.equals(argumentType)) {
            finalArguments[index] = arguments[index];
            continue;
          } else if (constParameterType.equals("float") && argumentType.equals("double")) {
            finalArguments[index] = ((Double) arguments[index]).floatValue();
            continue;
          } else if (argumentType.equals("int")) {
            if (constParameterType.equals("byte")) {
              finalArguments[index] = ((Integer) arguments[index]).byteValue();
              continue;
            } else if (constParameterType.equals("short")) {
              finalArguments[index] = ((Integer) arguments[index]).shortValue();
              continue;
            }
          } else if (argumentType.equals("string")) {
            if (constParameterType.equals("char") && ((String) arguments[index]).length() == 1) {
              finalArguments[index] = ((String) arguments[index]).charAt(0);
              continue;
            }
          }

          continue methodLoop;
        } else {
          if (arguments[index] == null) {
            finalArguments[index] = null;
            continue;
          }

          Class<?> argumentClass = arguments[index].getClass();
          if (methodParameterType.isAssignableFrom(argumentClass)) {
            finalArguments[index] = arguments[index];
          } else {
            continue methodLoop;
          }
        }
      }

      if (isUnsafeReflectionEnabled()) {
        try {
          method.setAccessible(true);
        } catch (SecurityException ignored) {
        }
      }

      return new Object[]{method.getReturnType(), method.getReturnType().getComponentType(), method.invoke(target, finalArguments)};
    }

    String argumentString = Arrays.toString(arguments);
    throw new IllegalArgumentException("Can't find method " + className + "." + methodName + "(" + argumentString.substring(1, argumentString.length() - 1) + ").");
  }

  public static Object getConstructor(String className, Object[] arguments) throws ClassNotFoundException, InvocationTargetException, IllegalAccessException, InstantiationException {
    Class<?> clazz = getClassLoader().loadClass(className);
    Constructor<?>[] constructors = clazz.getDeclaredConstructors();
    final Object[] finalArguments = new Object[arguments.length];

    constructorLoop:
    for (Constructor<?> constructor : constructors) {
      Class<?>[] constructorParameterTypes = constructor.getParameterTypes();
      if (arguments.length != constructorParameterTypes.length) {
        continue;
      }

      for (int index = 0; index < arguments.length; index++) {
        Class<?> constructorParameterType = constructorParameterTypes[index];
        if (constructorParameterType.isPrimitive()) {
          if (arguments[index] == null) {
            continue constructorLoop;
          }

          String constParameterType = constructorParameterType.getName();
          String argumentType = getReferencePrimitiveType(arguments[index].getClass());
          if (argumentType == null) {
            continue constructorLoop;
          }

          if (constParameterType.equals(argumentType)) {
            finalArguments[index] = arguments[index];
            continue;
          } else if (constParameterType.equals("float") && argumentType.equals("double")) {
            finalArguments[index] = ((Double) arguments[index]).floatValue();
            continue;
          } else if (argumentType.equals("int")) {
            if (constParameterType.equals("byte")) {
              finalArguments[index] = ((Integer) arguments[index]).byteValue();
              continue;
            } else if (constParameterType.equals("short")) {
              finalArguments[index] = ((Integer) arguments[index]).shortValue();
              continue;
            }
          } else if (argumentType.equals("string")) {
            if (constParameterType.equals("char") && ((String) arguments[index]).length() == 1) {
              finalArguments[index] = ((String) arguments[index]).charAt(0);
              continue;
            }
          }

          continue constructorLoop;
        } else {
          if (arguments[index] == null) {
            finalArguments[index] = null;
            continue;
          }

          Class<?> argumentClass = arguments[index].getClass();
          if (constructorParameterType.isAssignableFrom(argumentClass)) {
            finalArguments[index] = arguments[index];
          } else {
            continue constructorLoop;
          }
        }
      }

      if (isUnsafeReflectionEnabled()) {
        try {
          constructor.setAccessible(true);
        } catch (SecurityException ignored) {
        }
      }
      return constructor.newInstance(finalArguments);
    }

    String argumentString = Arrays.toString(arguments);
    throw new IllegalArgumentException("Can't find constructor " + className + "(" + argumentString.substring(1, argumentString.length() - 1) + ").");
  }

  private static String getReferencePrimitiveType(Class<?> referenceClazz) {
    switch (referenceClazz.getName()) {
      case "java.lang.Integer":
        return "int";
      case "java.lang.Long":
        return "long";
      case "java.lang.Double":
        return "double";
      case "java.lang.Boolean":
        return "boolean";
      case "java.lang.String":
        return "string";
      default:
        return null;
    }
  }

}
