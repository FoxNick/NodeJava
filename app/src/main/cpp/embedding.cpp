#include "embedding.h"
#include "main.h"
#include "log.h"
#include "javabridge/ClassInfo.h"
#include "Util.h"

v8::Local<v8::Value> getClassInfo(
        v8::Isolate *isolate,
        v8::Local<v8::Context> context,
        v8::Local<v8::String> className
) {
    JNIEnv *env = Main::env();
    jclass javaBridgeUtilClass = env->FindClass("com/mucheng/nodejava/javabridge/JavaBridgeUtil");
    jmethodID findClassOrNull = env->GetStaticMethodID(javaBridgeUtilClass, "findClassOrNull",
                                                       "(Ljava/lang/String;)Lcom/mucheng/nodejava/javabridge/ClassInfo;");
    jobject classInfoInstance = env->CallStaticObjectMethod(javaBridgeUtilClass, findClassOrNull,
                                                            env->NewStringUTF(
                                                                    *v8::String::Utf8Value(isolate,
                                                                                           className)));

    if (classInfoInstance == nullptr) {
        return v8::Null(isolate);
    }

    return ClassInfo::BuildObject(isolate, context, classInfoInstance);
}

void JAVA_ACCESSOR_BINDING(
        v8::Local<v8::Object> exports,
        v8::Local<v8::Value>,
        v8::Local<v8::Context> context,
        void *priv
) {
    v8::Isolate *isolate = context->GetIsolate();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "className"),
            v8::Symbol::For(isolate, v8::String::NewFromUtf8Literal(isolate, "className"))
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "getClassInfo"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                info.GetReturnValue().Set(getClassInfo(isolate, context, info[0].As<v8::String>()));
            }).ToLocalChecked()
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "setUnsafeReflectionEnabled"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                JNIEnv *env = Main::env();
                jclass javaBridgeUtilClass = env->FindClass(
                        "com/mucheng/nodejava/javabridge/JavaBridgeUtil");
                jmethodID setUnsafeReflectionEnabled = env->GetStaticMethodID(javaBridgeUtilClass,
                                                                              "setUnsafeReflectionEnabled",
                                                                              "(Z)V");
                env->CallStaticVoidMethod(javaBridgeUtilClass, setUnsafeReflectionEnabled,
                                          info[0].As<v8::Boolean>()->Value());
            }).ToLocalChecked()
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "isUnsafeReflectionEnabled"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                JNIEnv *env = Main::env();
                jclass javaBridgeUtilClass = env->FindClass(
                        "com/mucheng/nodejava/javabridge/JavaBridgeUtil");
                jmethodID isUnsafeReflectionEnabled = env->GetStaticMethodID(javaBridgeUtilClass,
                                                                             "isUnsafeReflectionEnabled",
                                                                             "()Z");
                info.GetReturnValue().Set(env->CallStaticBooleanMethod(javaBridgeUtilClass,
                                                                       isUnsafeReflectionEnabled));
            }).ToLocalChecked()
    ).Check();

}
