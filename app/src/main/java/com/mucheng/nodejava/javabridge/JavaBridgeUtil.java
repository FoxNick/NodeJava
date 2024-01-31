package com.mucheng.nodejava.javabridge;

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

  public static Object getField(String className, String fieldName, Object target) throws ClassNotFoundException, NoSuchFieldException, IllegalAccessException {
    Class<?> clazz = getClassLoader().loadClass(className);
    Field field = clazz.getDeclaredField(fieldName);
    if (isUnsafeReflectionEnabled()) {
      try {
        field.setAccessible(true);
      } catch (SecurityException ignored) {
      }
    }

    return field.get(target);
  }

  public static void setField(String className, String fieldName, Object value, Object target) throws ClassNotFoundException, NoSuchFieldException, IllegalAccessException {
    Class<?> clazz = getClassLoader().loadClass(className);
    Field field = clazz.getDeclaredField(fieldName);

    Class<?> parameterType = field.getType();

    if (parameterType.isPrimitive()) {
      if (value == null) {
        throw new NoSuchFieldException("Can't set field " + className + "." + fieldName + " to " + argumentTypesToString(getArgumentTypes(new Object[]{null})));
      }
      parameterType = getReferenceType(parameterType);
    }

    if (value == null) {
      field.set(target, null);
      return;
    } else {

      Class<?> argumentType = value.getClass();
      String parameterTypeClassName = parameterType.getName();
      String argumentTypeClassName = argumentType.getName();

      if (parameterTypeClassName.equals("java.lang.Character")) {
        if (argumentTypeClassName.equals("java.lang.String") && ((String) value).length() == 1) {
          field.set(target, ((String) value).charAt(0));
          return;
        }
      }

      if (parameterType.isAssignableFrom(argumentType)) {
        field.set(target, value);
        return;
      }

      if (parameterTypeClassName.equals("java.lang.Byte")) {
        if (argumentTypeClassName.equals("java.lang.Integer")) {
          field.set(target, ((Integer) value).byteValue());
          return;
        }
      }

      if (parameterTypeClassName.equals("java.lang.Short")) {
        if (argumentTypeClassName.equals("java.lang.Integer")) {
          field.set(target, ((Integer) value).shortValue());
          return;
        }
      }

      if (parameterTypeClassName.equals("java.lang.Float")) {
        if (argumentTypeClassName.equals("java.lang.Double")) {
          field.set(target, ((Double) value).floatValue());
          return;
        }
      }
    }

    if (isUnsafeReflectionEnabled()) {
      try {
        field.setAccessible(true);
      } catch (SecurityException ignored) {
      }
    }

    throw new NoSuchFieldException("Can't set field " + className + "." + fieldName + " to " + argumentTypesToString(getArgumentTypes(new Object[]{value})));
  }

  public static Object[] callMethod(String className, String methodName, Object[] arguments, Object target) throws ClassNotFoundException, NoSuchMethodException, InvocationTargetException, IllegalAccessException {
    Class<?> clazz = getClassLoader().loadClass(className);
    Method[] methods = clazz.getDeclaredMethods();
    Object[] handledParams = new Object[arguments.length];

    findMethodLoop:
    for (Method method : methods) {

      if (method.getParameterTypes().length != arguments.length || !method.getName().equals(methodName)) {
        continue;
      }

      for (int index = 0; index < method.getParameterTypes().length; index++) {
        Class<?> parameterType = method.getParameterTypes()[index];
        Object argument = arguments[index];

        if (parameterType.isPrimitive()) {
          if (argument == null) {
            continue findMethodLoop;
          }
          parameterType = getReferenceType(parameterType);
        }

        if (argument == null) {
          handledParams[index] = null;
          continue;
        }

        Class<?> argumentType = argument.getClass();
        String parameterTypeClassName = parameterType.getName();
        String argumentTypeClassName = argumentType.getName();

        if (parameterTypeClassName.equals("java.lang.Character")) {
          if (argumentTypeClassName.equals("java.lang.String") && ((String) argument).length() == 1) {
            handledParams[index] = ((String) argument).charAt(0);
            continue;
          }
        }

        if (parameterType.isAssignableFrom(argumentType)) {
          handledParams[index] = argument;
          continue;
        }

        if (parameterTypeClassName.equals("java.lang.Byte")) {
          if (argumentTypeClassName.equals("java.lang.Integer")) {
            handledParams[index] = ((Integer) argument).byteValue();
            continue;
          }
        }

        if (parameterTypeClassName.equals("java.lang.Short")) {
          if (argumentTypeClassName.equals("java.lang.Integer")) {
            handledParams[index] = ((Integer) argument).shortValue();
            continue;
          }
        }

        if (parameterTypeClassName.equals("java.lang.Float")) {
          if (argumentTypeClassName.equals("java.lang.Double")) {
            handledParams[index] = ((Double) argument).floatValue();
            continue;
          }
        }

        continue findMethodLoop;
      }

      if (isUnsafeReflectionEnabled()) {
        try {
          method.setAccessible(true);
        } catch (SecurityException ignored) {
        }
      }

      return new Object[]{method.invoke(target, handledParams), isVoid(method)};

    }

    throw new NoSuchMethodException("Can't find method " + className + "." + methodName + "(" + argumentTypesToString(getArgumentTypes(arguments)) + ").");
  }

  public static Object getConstructor(String className, Object[] arguments) throws ClassNotFoundException, InvocationTargetException, IllegalAccessException, InstantiationException {
    Class<?> clazz = getClassLoader().loadClass(className);
    Constructor<?>[] constructors = clazz.getDeclaredConstructors();
    Object[] handledParams = new Object[arguments.length];

    findConstructorLoop:
    for (Constructor<?> constructor : constructors) {

      if (constructor.getParameterTypes().length != arguments.length) {
        continue;
      }

      for (int index = 0; index < constructor.getParameterTypes().length; index++) {
        Class<?> parameterType = constructor.getParameterTypes()[index];
        Object argument = arguments[index];

        if (parameterType.isPrimitive()) {
          if (argument == null) {
            continue findConstructorLoop;
          }
          parameterType = getReferenceType(parameterType);
        }

        if (argument == null) {
          handledParams[index] = null;
          continue;
        }

        Class<?> argumentType = argument.getClass();
        String parameterTypeClassName = parameterType.getName();
        String argumentTypeClassName = argumentType.getName();

        if (parameterTypeClassName.equals("java.lang.Character")) {
          if (argumentTypeClassName.equals("java.lang.String") && ((String) argument).length() == 1) {
            handledParams[index] = ((String) argument).charAt(0);
            continue;
          }
        }

        if (parameterType.isAssignableFrom(argumentType)) {
          handledParams[index] = argument;
          continue;
        }

        if (parameterTypeClassName.equals("java.lang.Byte")) {
          if (argumentTypeClassName.equals("java.lang.Integer")) {
            handledParams[index] = ((Integer) argument).byteValue();
            continue;
          }
        }

        if (parameterTypeClassName.equals("java.lang.Short")) {
          if (argumentTypeClassName.equals("java.lang.Integer")) {
            handledParams[index] = ((Integer) argument).shortValue();
            continue;
          }
        }

        if (parameterTypeClassName.equals("java.lang.Float")) {
          if (argumentTypeClassName.equals("java.lang.Double")) {
            handledParams[index] = ((Double) argument).floatValue();
            continue;
          }
        }

        continue findConstructorLoop;
      }

      if (isUnsafeReflectionEnabled()) {
        try {
          constructor.setAccessible(true);
        } catch (SecurityException ignored) {
        }
      }

      return constructor.newInstance(handledParams);

    }

    throw new InstantiationException("Can't find constructor " + className + "(" + argumentTypesToString(getArgumentTypes(arguments)) + ").");
  }

  private static String argumentTypesToString(Class<?>[] argumentTypes) {
    StringBuilder stringBuilder = new StringBuilder();
    for (int index = 0; index < argumentTypes.length; index++) {
      Class<?> argumentType = argumentTypes[index];
      if (argumentType == null) {
        stringBuilder.append("null");
      } else {
        stringBuilder.append(argumentTypes[index].getName());
      }
      if (index + 1 < argumentTypes.length) {
        stringBuilder.append(", ");
      }
    }
    return stringBuilder.toString();
  }

  private static Class<?> getReferenceType(Class<?> clazz) {
    if (!clazz.isPrimitive()) {
      return clazz;
    }

    switch (clazz.getName()) {
      case "void":
        return Void.class;
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
      case "double":
        return Double.class;
      case "char":
        return Character.class;
      default:
        return Boolean.class;
    }
  }

  private static boolean isVoid(Method method) {
    return method.getReturnType().getName().equals("void");
  }

  private static Class<?>[] getArgumentTypes(Object[] arguments) {
    Class<?>[] classes = new Class[arguments.length];
    for (int index = 0; index < classes.length; index++) {
      Object argument = arguments[index];
      if (argument == null) {
        classes[index] = null;
      } else {
        classes[index] = argument.getClass();
      }
    }
    return classes;
  }

}
