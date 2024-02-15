package com.mucheng.nodejava.javabridge;

import android.util.Log;

import com.android.dx.Code;
import com.android.dx.Comparison;
import com.android.dx.DexMaker;
import com.android.dx.FieldId;
import com.android.dx.Label;
import com.android.dx.Local;
import com.android.dx.MethodId;
import com.android.dx.TypeId;

import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import dalvik.system.PathClassLoader;

/**
 * @noinspection unused
 */
public final class JavaBridgeUtil {

    private static boolean unsafeReflectionEnabled;

    private static ClassLoader classLoader;

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
        if (classLoader == null) {
            classLoader = Thread.currentThread().getContextClassLoader();
        }
        return classLoader;
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

    public static void setField(String className, String fieldName, Object argument, Object target) throws ClassNotFoundException, NoSuchFieldException, IllegalAccessException {
        Class<?> clazz = getClassLoader().loadClass(className);
        Field field = clazz.getDeclaredField(fieldName);

        Class<?> parameterType = field.getType();

        if (parameterType.isPrimitive()) {
            if (argument == null) {
                throw new NoSuchFieldException("Can't set field " + className + "." + fieldName + " to " + argumentTypesToString(getArgumentTypes(new Object[]{null})));
            }
            parameterType = getReferenceType(parameterType);
        }

        if (argument == null) {
            field.set(target, null);
            return;
        } else {

            Class<?> argumentType = argument.getClass();
            String parameterTypeClassName = parameterType.getName();
            String argumentTypeClassName = argumentType.getName();

            if (parameterTypeClassName.equals("java.lang.Character")) {
                if (argumentTypeClassName.equals("java.lang.String") && ((String) argument).length() == 1) {
                    field.set(target, ((String) argument).charAt(0));
                    return;
                }
            }

            if (parameterType.isAssignableFrom(argumentType)) {
                field.set(target, argument);
                return;
            }

            if (parameterType.isInterface()) {
                if (argumentTypeClassName.equals("com.mucheng.nodejava.javabridge.Interface")) {
                    int methodCount = parameterType.getMethods().length;
                    final Interface asInterface = (Interface) argument;
                    if (methodCount > 1 && asInterface.isFunction()) {
                        throw new IllegalArgumentException();
                    }

                    final boolean isFunction = asInterface.isFunction();

                    field.set(target, Proxy.newProxyInstance(getClassLoader(), new Class[]{parameterType}, new InvocationHandler() {

                        @Override
                        public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
                            Thread currentThread = Thread.currentThread();

                            if (args == null) {
                                args = new Object[0];
                            }

                            if (isFunction) {
                                return asInterface.invoke(null, args);
                            }
                            return asInterface.invoke(method.getName(), args);
                        }

                    }));
                    return;
                }
            }

            if (parameterTypeClassName.equals("java.lang.Byte")) {
                if (argumentTypeClassName.equals("java.lang.Integer")) {
                    field.set(target, ((Integer) argument).byteValue());
                    return;
                }
            }

            if (parameterTypeClassName.equals("java.lang.Short")) {
                if (argumentTypeClassName.equals("java.lang.Integer")) {
                    field.set(target, ((Integer) argument).shortValue());
                    return;
                }
            }

            if (parameterTypeClassName.equals("java.lang.Float")) {
                if (argumentTypeClassName.equals("java.lang.Double")) {
                    field.set(target, ((Double) argument).floatValue());
                    return;
                }
            }

            if (parameterTypeClassName.equals("java.lang.Long")) {
                if (argumentTypeClassName.equals("java.lang.Integer")) {
                    field.set(target, ((Integer) argument).longValue());
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

        throw new NoSuchFieldException("Can't set field " + className + "." + fieldName + " to " + argumentTypesToString(getArgumentTypes(new Object[]{argument})));
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

                if (parameterType.isInterface()) {
                    if (argumentTypeClassName.equals("com.mucheng.nodejava.javabridge.Interface")) {
                        int methodCount = parameterType.getMethods().length;
                        final Interface asInterface = (Interface) argument;
                        if (methodCount > 1 && asInterface.isFunction()) {
                            throw new IllegalArgumentException();
                        }

                        final boolean isFunction = asInterface.isFunction();

                        handledParams[index] = Proxy.newProxyInstance(getClassLoader(), new Class[]{parameterType}, new InvocationHandler() {

                            @Override
                            public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {

                                if (args == null) {
                                    args = new Object[0];
                                }

                                if (isFunction) {
                                    return asInterface.invoke(null, args);
                                }
                                return asInterface.invoke(method.getName(), args);
                            }

                        });
                        continue;
                    }
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

                if (parameterTypeClassName.equals("java.lang.Long")) {
                    if (argumentTypeClassName.equals("java.lang.Integer")) {
                        handledParams[index] = ((Integer) argument).longValue();
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

                if (parameterType.isInterface()) {
                    if (argumentTypeClassName.equals("com.mucheng.nodejava.javabridge.Interface")) {
                        int methodCount = parameterType.getMethods().length;
                        final Interface asInterface = (Interface) argument;
                        if (methodCount > 1 && asInterface.isFunction()) {
                            throw new IllegalArgumentException();
                        }

                        final boolean isFunction = asInterface.isFunction();

                        handledParams[index] = Proxy.newProxyInstance(getClassLoader(), new Class[]{parameterType}, new InvocationHandler() {

                            @Override
                            public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {

                                if (args == null) {
                                    args = new Object[0];
                                }

                                if (isFunction) {
                                    return asInterface.invoke(null, args);
                                }
                                return asInterface.invoke(method.getName(), args);
                            }

                        });
                        continue;
                    }
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

                if (parameterTypeClassName.equals("java.lang.Long")) {
                    if (argumentTypeClassName.equals("java.lang.Integer")) {
                        handledParams[index] = ((Integer) argument).longValue();
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

        if (clazz.getSuperclass() != null) {
            return getConstructor(clazz.getSuperclass().getName(), arguments);
        }
        throw new InstantiationException("Can't find constructor " + className + "(" + argumentTypesToString(getArgumentTypes(arguments)) + ").");
    }

    private static String argumentTypesToString(Class<?>[] argumentTypes) {
        StringBuilder stringBuilder = new StringBuilder();
        for (int index = 0; index < argumentTypes.length; index++) {
            Class<?> argumentType = argumentTypes[index];
            if (argumentType == null) {
                stringBuilder.append("null");
            } else if (argumentType.getName().equals("com.mucheng.nodejava.javabridge.Interface")) {
                stringBuilder.append("interface");
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

    private static void defineClass(
            String className,
            String superclass,
            String[] implementations,
            String[] methods,
            String outputDexFile
    ) throws ClassNotFoundException, IOException {
        Class<?> superJavaClass = getClassLoader().loadClass(superclass);

        DexMaker dexMaker = new DexMaker();

        TypeId currentClass = TypeId.get("L" + className.replaceAll("\\.", "/") + ";");
        TypeId superClass = TypeId.get(superJavaClass);
        Class<?>[] interfaceClasses = new Class[implementations.length + 1];
        TypeId[] interfaces = new TypeId[implementations.length + 1];
        for (int index = 0; index < implementations.length; index++) {
            interfaceClasses[index] = classLoader.loadClass(implementations[index]);
            interfaces[index] = TypeId.get(interfaceClasses[index]);
        }
        interfaceClasses[interfaceClasses.length - 1] = JavaScriptGeneratedClass.class;
        interfaces[interfaces.length - 1] = TypeId.get(JavaScriptGeneratedClass.class);
        dexMaker.declare(currentClass, "", Modifier.PUBLIC, superClass, interfaces);
        generatePtrField(dexMaker, currentClass);
        generateMethods(dexMaker, currentClass, methods, interfaceClasses, superJavaClass);

        byte[] byteArray = dexMaker.generate();

        FileOutputStream fileOutputStream = new FileOutputStream(outputDexFile);
        fileOutputStream.write(byteArray);
        fileOutputStream.flush();
        fileOutputStream.close();

        loadDex(outputDexFile);
    }

    private static void generatePtrField(DexMaker dexMaker, TypeId currentClass) {
        FieldId fieldId = currentClass.getField(TypeId.LONG, "javaScriptDelegatePtr");
        dexMaker.declare(fieldId, Modifier.PRIVATE | Modifier.FINAL, null);
    }

    private static void generateMethods(DexMaker dexMaker, TypeId currentClass, String[] methodNames, Class<?>[] interfaces, Class<?> superJavaClass) {

    }

    private static void loadDex(String dex) {
        classLoader = new PathClassLoader(dex, classLoader);
    }

}
