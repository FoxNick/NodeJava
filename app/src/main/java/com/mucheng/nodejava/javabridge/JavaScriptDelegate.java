package com.mucheng.nodejava.javabridge;

import android.util.Log;

public final class JavaScriptDelegate {

    private final long interfacePtr;
    private final Object javaObject;

    public JavaScriptDelegate(long interfacePtr, Object javaObject) {
        this.interfacePtr = interfacePtr;
        this.javaObject = javaObject;
    }

    public boolean hasMethod(String methodName) {
        return nativeHasMethod(interfacePtr, methodName);
    }

    public boolean isInteractingJavaMethod() {
        return nativeIsInteractingJavaMethod(interfacePtr);
    }

    public Object callMethod(String methodName, Object[] arguments) {
        return nativeCallMethod(interfacePtr, javaObject, methodName, arguments);
    }

    private native boolean nativeHasMethod(long interfacePtr, String methodName);

    private native boolean nativeIsInteractingJavaMethod(long interfacePtr);

    private native Object nativeCallMethod(long interfacePtr, Object javaObject, String methodName, Object[] arguments);

}
