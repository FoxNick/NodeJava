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
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

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
        generateJavaScriptDelegateField(dexMaker, currentClass);
        generateConstructors(dexMaker, currentClass, superJavaClass, superClass);
        generateMethods(dexMaker, currentClass, methods, interfaceClasses, superJavaClass);

        byte[] byteArray = dexMaker.generate();

        FileOutputStream fileOutputStream = new FileOutputStream(outputDexFile);
        fileOutputStream.write(byteArray);
        fileOutputStream.flush();
        fileOutputStream.close();

        loadDex(outputDexFile);
    }

    private static void generateConstructors(DexMaker dexMaker, TypeId currentClass, Class<?> superJavaClass, TypeId superClass) {
        TypeId javaScriptDelegateTypeId = TypeId.get(JavaScriptDelegate.class);
        Constructor<?>[] constructors = getOverridableConstructors(superJavaClass).toArray(new Constructor[0]);
        for (Constructor<?> constructor : constructors) {
            Class<?>[] parameterTypes = constructor.getParameterTypes();
            TypeId[] parameterTypeIds = classes2typeIds(parameterTypes);
            TypeId[] newParameterTypeIds = new TypeId[parameterTypes.length + 1];
            System.arraycopy(parameterTypeIds, 0, newParameterTypeIds, 0, parameterTypes.length);
            newParameterTypeIds[parameterTypes.length] = TypeId.LONG;

            MethodId superInitMethod = superClass.getConstructor(parameterTypeIds);
            MethodId initMethod = currentClass.getConstructor(newParameterTypeIds);
            FieldId javaScriptDelegateFieldId = currentClass.getField(TypeId.get(JavaScriptDelegate.class), "javaScriptDelegate");
            MethodId javaScriptDelegateInitMethod = javaScriptDelegateTypeId.getConstructor(TypeId.LONG, TypeId.OBJECT);
            Code code = dexMaker.declare(initMethod, Modifier.PUBLIC);
            Local javaScriptDelegateLocal = code.newLocal(javaScriptDelegateTypeId);
            Local thisLocal = code.getThis(currentClass);
            Local[] parameterLocals = new Local[parameterTypes.length];
            for (int index = 0; index < parameterTypes.length; index++) {
                parameterLocals[index] = code.getParameter(index, parameterTypeIds[index]);
            }
            code.invokeDirect(superInitMethod, null, thisLocal, parameterLocals);
            code.newInstance(javaScriptDelegateLocal, javaScriptDelegateInitMethod, code.getParameter(parameterTypes.length, TypeId.LONG), thisLocal);
            code.iput(javaScriptDelegateFieldId, thisLocal, javaScriptDelegateLocal);
            code.returnVoid();
        }
    }

    private static void generateJavaScriptDelegateField(DexMaker dexMaker, TypeId currentClass) {
        FieldId fieldId = currentClass.getField(TypeId.get(JavaScriptDelegate.class), "javaScriptDelegate");
        dexMaker.declare(fieldId, Modifier.PRIVATE | Modifier.FINAL, null);
    }

    private static void generateMethods(DexMaker dexMaker, TypeId currentClass, String[] methodNames, Class<?>[] interfaces, Class<?> superJavaClass) {
        TypeId javaScriptDelegateTypeId = TypeId.get(JavaScriptDelegate.class);
        List<Method> methods = getOverridableMethods(superJavaClass, methodNames, interfaces);
        for (Method method : methods) {
            Class<?>[] parameterTypes = method.getParameterTypes();
            TypeId[] parameterTypeIds = classes2typeIds(parameterTypes);
            MethodId methodId = currentClass.getMethod(TypeId.get(method.getReturnType()), method.getName(), parameterTypeIds);
            FieldId javaScriptDelegateFieldId = currentClass.getField(javaScriptDelegateTypeId, "javaScriptDelegate");
            Code code = dexMaker.declare(methodId, Modifier.PUBLIC);
            Local javaScriptDelegateLocal = code.newLocal(javaScriptDelegateTypeId);
            Local nullLocal = code.newLocal(TypeId.OBJECT);
            Local thisLocal = code.getThis(currentClass);
            Local[] parameterLocals = new Local[parameterTypes.length];
            for (int index = 0; index < parameterTypes.length; index++) {
                parameterLocals[index] = code.getParameter(index, parameterTypeIds[index]);
            }
            code.loadConstant(nullLocal, null);
            code.iget(javaScriptDelegateFieldId, javaScriptDelegateLocal, thisLocal);
            Label isNullLabel = new Label();
            code.compare(Comparison.NE, isNullLabel, javaScriptDelegateLocal, nullLocal);
            // If this object is null
            code.mark(isNullLabel);

            // Else

            code.returnVoid();
        }
    }

    private static TypeId[] classes2typeIds(Class<?>[] classes) {
        TypeId[] typeIds = new TypeId[classes.length];
        for (int index = 0; index < classes.length; index++) {
            typeIds[index] = TypeId.get(classes[index]);
        }
        return typeIds;
    }

    private static void loadDex(String dex) {
        classLoader = new PathClassLoader(dex, classLoader);
    }

    private static List<Constructor<?>> getOverridableConstructors(Class<?> clazz) {
        List<Constructor<?>> list = new ArrayList<>();
        Constructor<?>[] declaredConstructors = clazz.getDeclaredConstructors();
        for (Constructor<?> declaredConstructor : declaredConstructors) {
            int modifiers = declaredConstructor.getModifiers();
            if (!Modifier.isPublic(modifiers) && !Modifier.isProtected(modifiers)) {
                continue;
            }
            list.add(declaredConstructor);
        }
        return list;
    }

    private static List<Method> getOverridableMethods(Class<?> clazz, String[] methodNames, Class<?>[] interfaces) {
        return getOverridableMethods(clazz, methodNames, interfaces, new HashMap<>());
    }

    private static List<Method> getOverridableMethods(Class<?> clazz, String[] methodNames, Class<?>[] interfaces, Map<String, Set<String>> map) {
        Objects.requireNonNull(map);
        Method[] classDeclaredMethods = clazz.getDeclaredMethods();
        List<Method> methods = new ArrayList<>();

        for (String methodName : methodNames) {
            if (!map.containsKey(methodName)) {
                map.put(methodName, new HashSet<>());
            }

            for (Method method : classDeclaredMethods) {
                if (!method.getName().equals(methodName)) {
                    continue;
                }

                int modifiers = method.getModifiers();
                if (Modifier.isFinal(modifiers) || (!Modifier.isPublic(modifiers) && !Modifier.isProtected(modifiers))) {
                    continue;
                }

                Set<String> set = map.get(methodName);
                if (set != null) {
                    if (set.add(Arrays.toString(method.getParameterTypes()))) {
                        methods.add(method);
                    }
                }
            }

            if (clazz.getSuperclass() != null) {
                methods.addAll(getOverridableMethods(clazz.getSuperclass(), methodNames, interfaces, map));
            }

            for (Class<?> theInterface : interfaces) {
                Method[] interfaceDeclaredMethods = theInterface.getDeclaredMethods();
                for (Method method : interfaceDeclaredMethods) {
                    if (!method.getName().equals(methodName)) {
                        continue;
                    }

                    int modifiers = method.getModifiers();
                    if (Modifier.isFinal(modifiers) || (!Modifier.isPublic(modifiers) && !Modifier.isProtected(modifiers))) {
                        continue;
                    }

                    Set<String> set = map.get(methodName);
                    if (set != null) {
                        if (set.add(Arrays.toString(method.getParameterTypes()))) {
                            methods.add(method);
                        }
                    }
                }
            }

        }

        return methods;
    }

}
