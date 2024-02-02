package com.mucheng.nodejava.javabridge;

public class Interface {

  long interfacePtr;

  public Object invoke(String methodName, Object[] params) {
    return nativeInvoke(methodName, params);
  }

  public boolean isFunction() {
    return nativeIsFunction();
  }

  private native Object nativeInvoke(String methodName, Object[] params);

  public native boolean nativeIsFunction();

}
