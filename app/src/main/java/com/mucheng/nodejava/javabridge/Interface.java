package com.mucheng.nodejava.javabridge;

public class Interface {

  long interfacePtr;

  public Object invoke(Object[] params) {
    return nativeInvoke(params);
  }

  public Object invoke(String methodName, Object[] params) {
    return nativeInvoke(methodName, params);
  }

  public boolean isFunction() {
    return nativeIsFunction();
  }

  private native Object nativeInvoke(Object[] params);

  private native Object nativeInvoke(String methodName, Object[] params);

  public native boolean nativeIsFunction();

}
