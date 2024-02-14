package com.mucheng.nodejava.core;

import android.os.Handler;
import android.os.Looper;

import java.util.Timer;
import java.util.TimerTask;

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

    public void spinEventLoopNoWait() {
        if (Looper.getMainLooper().equals(Looper.myLooper())) {
            final Handler handler = new Handler(Looper.getMainLooper());
            handler.post(new Runnable() {
                @Override
                public void run() {
                    if (nativeUvRunNoWait()) {
                        nativeDrainTasks();
                        handler.postDelayed(this, nativeUvBackendTimeout());
                    } else {
                        handler.postDelayed(this, 3);
                    }
                }
            });
        } else {
            new Timer().schedule(new TimerTask() {

                @Override
                public void run() {
                    final Timer timer = new Timer();
                    if (nativeUvRunNoWait()) {
                        nativeDrainTasks();
                        timer.schedule(this, nativeUvBackendTimeout());
                    } else {
                        timer.schedule(this, 3);
                    }
                }

            }, 0);
        }
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

    private native boolean nativeUvRunNoWait();

    private native void nativeDrainTasks();

    private native long nativeUvBackendTimeout();

    private native void nativeStop();

    private native void nativeInjectJavaBridge();

    public Isolate getIsolate() {
        return isolate;
    }

}
