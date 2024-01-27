#include <jni.h>
#include "Util.h"
#include "main.h"

void Util::SetPtr(jclass javaClass, const char *fieldName, void *ptr) {
    jfieldID field = Main::env()->GetStaticFieldID(javaClass, fieldName, "J");
    Main::env()->SetStaticLongField(javaClass, field, reinterpret_cast<jlong>(ptr));
}

void Util::SetPtr(jobject instance, const char *fieldName, void *ptr) {
    jclass javaClass = Main::env()->GetObjectClass(instance);
    jfieldID field = Main::env()->GetFieldID(javaClass, fieldName, "J");
    Main::env()->SetLongField(instance, field, reinterpret_cast<jlong>(ptr));
}

void *Util::GetPtr(jclass javaClass, const char *fieldName) {
    jfieldID field = Main::env()->GetStaticFieldID(javaClass, fieldName, "J");
    return reinterpret_cast<void *>(Main::env()->GetStaticLongField(javaClass, field));
}

void *Util::GetPtr(jobject instance, const char *fieldName) {
    jclass javaClass = Main::env()->GetObjectClass(instance);
    jfieldID field = Main::env()->GetFieldID(javaClass, fieldName, "J");
    return reinterpret_cast<void *>(Main::env()->GetLongField(instance, field));
}

const char *Util::JavaStr2CStr(jstring javaStr) {
    return Main::env()->GetStringUTFChars(javaStr, nullptr);
}

jstring Util::CStr2JavaStr(const char *cStr) {
    return Main::env()->NewStringUTF(cStr);
}

void Util::ThrowScriptCompilingException(const char *message) {
    Main::env()->ThrowNew(
            Main::env()->FindClass("com/mucheng/nodejava/core/exception/ScriptCompilingException"),
            message
    );
}

void Util::ThrowScriptRuntimeException(const char *message) {
    Main::env()->ThrowNew(
            Main::env()->FindClass("com/mucheng/nodejava/core/exception/ScriptRuntimeException"),
            message
    );
}

void Util::ThrowNodeException(const char *message) {
    Main::env()->ThrowNew(
            Main::env()->FindClass("com/mucheng/nodejava/core/exception/NodeException"),
            message
    );
}

void Util::ThrowExceptionToJS(v8::Isolate *isolate, jthrowable throwable) {
    JNIEnv *env = Main::env();
    jclass throwableClass = env->FindClass("java/lang/Throwable");
    jclass stringWriterClass = env->FindClass("java/io/StringWriter");
    jclass printWriterClass = env->FindClass("java/io/PrintWriter");
    jclass writerClass = env->FindClass("java/io/Writer");
    jmethodID stringWriterInit = env->GetMethodID(stringWriterClass, "<init>", "()V");
    jmethodID printWriterInit = env->GetMethodID(printWriterClass, "<init>", "(Ljava/io/Writer;)V");
    jmethodID printStackTrace = env->GetMethodID(throwableClass, "printStackTrace",
                                                 "(Ljava/io/PrintWriter;)V");
    jmethodID toString = env->GetMethodID(stringWriterClass, "toString", "()Ljava/lang/String;");
    jmethodID close = env->GetMethodID(writerClass, "close", "()V");

    jobject stringWriter = env->NewObject(stringWriterClass, stringWriterInit);
    jobject printWriter = env->NewObject(printWriterClass, printWriterInit, stringWriter);

    env->CallVoidMethod(throwable, printStackTrace, printWriter);
    jobject stackTrace = env->CallObjectMethod(stringWriter, toString);

    env->CallVoidMethod(printWriter, close);
    env->CallVoidMethod(stringWriter, close);

    isolate->ThrowError(v8::String::NewFromUtf8(isolate, Util::JavaStr2CStr(
            static_cast<jstring>(stackTrace))).ToLocalChecked());
}

v8::Local<v8::External> Util::NewExternal(v8::Isolate *isolate, void *value) {
    return v8::External::New(isolate, value);
}
