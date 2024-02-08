#include "Context.h"
#include "Isolate.h"
#include "Util.h"
#include "main.h"
#include "log.h"
#include "embedding.h"
#include "uv.h"
#include <jni.h>
#include <sstream>
#include <iostream>

Context::Context(Isolate *isolate) {
    this->isolate = isolate;

    SETUP_ISOLATE_CLASS();
    self.Reset(isolate->self, node::NewContext(isolate->self));

    environment = node::CreateEnvironment(
            isolate->isolateData,
            self.Get(isolate->self),
            Main::initializationResult->args(),
            Main::initializationResult->exec_args(),
            node::EnvironmentFlags::kOwnsProcessState
    );

    v8::Context::Scope contextScope(self.Get(isolate->self));
    SetProcessExitHandler(environment, [&](node::Environment *pEnvironment, int exitCode) {
        // Prevent Node.js to terminate isolate
        Stop(pEnvironment, node::StopFlags::kDoNotTerminateIsolate);
    });
}

void Context::To(jobject instance, Context *self) {
    Util::SetPtr(instance, "contextPtr", self);
}

Context *Context::From(jobject instance) {
    return Util::GetPtrAs<Context *>(instance, "contextPtr");
}

jobject getApplicationContext() {
    JNIEnv *env = Main::env();
    jclass activityThread = env->FindClass("android/app/ActivityThread");
    jmethodID currentActivityThread = env->GetStaticMethodID(activityThread,
                                                             "currentActivityThread",
                                                             "()Landroid/app/ActivityThread;");
    jobject at = env->CallStaticObjectMethod(activityThread, currentActivityThread);

    jmethodID getApplication = env->GetMethodID(activityThread, "getApplication",
                                                "()Landroid/app/Application;");
    jobject context = env->CallObjectMethod(at, getApplication);
    return context;
}

jstring getFilesDirAbsolutePath() {
    JNIEnv *env = Main::env();
    jclass contextClass = env->FindClass("android/content/Context");
    jclass fileClass = env->FindClass("java/io/File");
    jmethodID getFilesDir = env->GetMethodID(contextClass, "getFilesDir", "()Ljava/io/File;");
    jmethodID getAbsolutePath = env->GetMethodID(fileClass, "getAbsolutePath",
                                                 "()Ljava/lang/String;");
    jobject context = getApplicationContext();
    jobject filesDir = env->CallObjectMethod(context, getFilesDir);
    return static_cast<jstring>(env->CallObjectMethod(filesDir, getAbsolutePath));
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mucheng_nodejava_core_Context_nativeCreateContext(JNIEnv *env, jobject thiz,
                                                           jlong isolatePtr) {
    Isolate *isolate = Util::As<Isolate *>(isolatePtr);
    Context *context = new Context(isolate);
    Context::To(thiz, context);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mucheng_nodejava_core_Context_nativeLoadEnvironment(JNIEnv *env, jobject thiz,
                                                             jstring pwd) {
    Context *context = Context::From(thiz);
    Isolate *isolate = context->isolate;

    SETUP_ISOLATE_CLASS();
    SETUP_CONTEXT_CLASS();

    v8::TryCatch tryCatch(isolate->self);
    v8::MaybeLocal<v8::Value> result;
    if (pwd == nullptr) {
        std::string cPwd = Util::JavaStr2CStr(getFilesDirAbsolutePath());
        std::string chdir = std::string("process.chdir('" + cPwd + "');");
        std::string require = std::string(
                "globalThis.require = require('module').createRequire(process.cwd() + '/');");
        result = LoadEnvironment(context->environment, (chdir + require).c_str());
    } else {
        std::string cPwd = Util::JavaStr2CStr(pwd);
        std::string chdir = std::string("process.chdir('" + cPwd + "');");
        std::string require = std::string(
                "globalThis.require = require('module').createRequire(process.cwd() + '/');");
        result = LoadEnvironment(context->environment, (chdir + require).c_str());
    }

    if (result.IsEmpty()) {
        if (tryCatch.HasCaught()) {
            v8::MaybeLocal<v8::Value> stackTrace = v8::TryCatch::StackTrace(
                    context->self.Get(isolate->self), tryCatch.Exception());
            if (stackTrace.IsEmpty()) {
                Util::ThrowNodeException(
                        *v8::String::Utf8Value(isolate->self, tryCatch.Exception()));
            } else {
                Util::ThrowNodeException(
                        *v8::String::Utf8Value(isolate->self, stackTrace.ToLocalChecked()));
            }
        }
        return;
    }

    // Setup default Uncaught Exception Handler
    v8::Local<v8::Object> globalObject = context->self.Get(isolate->self)->Global();
    v8::Local<v8::Object> processObject = globalObject->Get(
            context->self.Get(isolate->self),
            v8::String::NewFromUtf8Literal(isolate->self,
                                           "process")).ToLocalChecked().As<v8::Object>();

    v8::Local<v8::Function> onFunction = processObject->Get(context->self.Get(isolate->self),
                                                            v8::String::NewFromUtf8Literal(
                                                                    isolate->self,
                                                                    "on")).ToLocalChecked().As<v8::Function>();

    onFunction->Call(context->self.Get(isolate->self), processObject, 2, new v8::Local<v8::Value>[]{
            v8::String::NewFromUtf8Literal(isolate->self, "uncaughtException"),
            v8::Function::New(context->self.Get(isolate->self),
                              [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                                  v8::Isolate *isolate = info.GetIsolate();
                                  v8::Local<v8::Object> err = info[0].As<v8::Object>();
                                  const char *cStr = *v8::String::Utf8Value(isolate, err);
                                  LOGE("Uncaught Exception: %s", cStr);
                                  Util::ThrowNodeException(cStr);
                              }).ToLocalChecked()
    }).ToLocalChecked();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_mucheng_nodejava_core_Context_nativeSpinEventLoop(JNIEnv *env, jobject thiz) {
    Context *context = Context::From(thiz);
    Isolate *isolate = context->isolate;

    SETUP_ISOLATE_CLASS();

    return SpinEventLoop(context->environment).FromMaybe(1) == 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mucheng_nodejava_core_Context_nativeStop(JNIEnv *env, jobject thiz) {
    Context *context = Context::From(thiz);
    Stop(context->environment, node::StopFlags::kDoNotTerminateIsolate);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mucheng_nodejava_core_Context_nativeEvaluateScript(JNIEnv *env, jobject thiz,
                                                            jstring script) {
    Context *context = Context::From(thiz);
    Isolate *isolate = context->isolate;

    SETUP_ISOLATE_CLASS();
    SETUP_CONTEXT_CLASS();

    v8::TryCatch tryCatch(isolate->self);
    v8::MaybeLocal<v8::Script> compiling = v8::Script::Compile(context->self.Get(isolate->self),
                                                               v8::String::NewFromUtf8(
                                                                       isolate->self,
                                                                       Util::JavaStr2CStr(
                                                                               script)).ToLocalChecked());
    if (compiling.IsEmpty()) {
        return;
    }

    v8::MaybeLocal<v8::Value> running = compiling.ToLocalChecked()->Run(
            context->self.Get(isolate->self));

    if (running.IsEmpty()) {
        return;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mucheng_nodejava_core_Context_nativeInjectJavaBridge(JNIEnv *env, jobject thiz) {
    Context *context = Context::From(thiz);
    AddLinkedBinding(
            context->environment,
            "java",
            JAVA_ACCESSOR_BINDING,
            nullptr
    );
}