package com.mucheng.nodejava.core;

public class Context {

  private final Isolate isolate;

  long contextPtr;

  public Context(Isolate isolate) {
    this(isolate, "");
  }

  public Context(Isolate isolate, String pwd) {
    this.isolate = isolate;
    nativeCreateContext(isolate.isolatePtr);
    nativeLoadEnvironment(pwd);
  }

  public static Context newRequiredContext(Isolate isolate, String pwd) {
    return new Context(isolate, "globalThis.require = require('module').createRequire('" + pwd + "/');");
  }

  public void evaluateScript(String script) {
    if (script == null) {
      throw new NullPointerException("script cannot be null");
    }
    nativeEvaluateScript(script);
  }

  public boolean spinEventLoop() {
    return nativeSpinEventLoop();
  }

  public void stop() {
    nativeStop();
  }

  public void injectJavaBridge() {
    nativeInjectJavaBridge();
  }

  private native void nativeCreateContext(long isolatePtr);

  private native void nativeLoadEnvironment(String pwd);

  private native void nativeEvaluateScript(String script);

  private native boolean nativeSpinEventLoop();

  private native void nativeStop();

  private native void nativeInjectJavaBridge();

  public Isolate getIsolate() {
    return isolate;
  }

}
