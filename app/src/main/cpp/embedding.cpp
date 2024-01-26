#include "embedding.h"
#include "main.h"
#include "log.h"
#include "javabridge/ClassInfo.h"
#include "javabridge/Accessor.h"
#include "Util.h"

v8::Local<v8::Object> getClassInfo(
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


    return ClassInfo::BuildObject(isolate, context, classInfoInstance);
}

v8::Local<v8::Object> getField(
        v8::Isolate *isolate,
        v8::Local<v8::Context> context,
        v8::Local<v8::Value> self,
        v8::Local<v8::String> fieldName
) {
    return Accessor::BuildFieldObject(isolate, context, self, fieldName);
}

v8::Local<v8::Object> callMethod(
        v8::Isolate *isolate,
        v8::Local<v8::Context> context,
        v8::Local<v8::Value> self,
        v8::Local<v8::String> methodName,
        v8::Local<v8::Array> args
) {
    return Accessor::BuildMethodObject(isolate, context, self, methodName, args);
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
            v8::String::NewFromUtf8Literal(isolate, "getField"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                info.GetReturnValue().Set(
                        getField(isolate, context, info[0], info[1].As<v8::String>()));
            }).ToLocalChecked()
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "callMethod"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                info.GetReturnValue().Set(
                        callMethod(isolate, context, info[0], info[1].As<v8::String>(), info[2].As<v8::Array>()));
            }).ToLocalChecked()
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "__classForName"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                JNIEnv *env = Main::env();
                v8::Local<v8::String> className = info[0].As<v8::String>();
                jclass objectClass = env->FindClass("java/lang/Object");
                jclass javaBridgeUtilClass = env->FindClass(
                        "com/mucheng/nodejava/javabridge/JavaBridgeUtil");
                jmethodID classForName = env->GetStaticMethodID(javaBridgeUtilClass, "classForName",
                                                                "(Ljava/lang/String;)Ljava/lang/Class;");
                jmethodID getClass = env->GetMethodID(objectClass, "getClass",
                                                      "()Ljava/lang/Class;");
                jobject javaClassObject = env->CallStaticObjectMethod(javaBridgeUtilClass,
                                                                      classForName,
                                                                      Util::CStr2JavaStr(
                                                                              *v8::String::Utf8Value(
                                                                                      isolate,
                                                                                      className)));

                if (env->ExceptionCheck()) {
                    jthrowable throwable = env->ExceptionOccurred();
                    env->ExceptionClear();
                    Util::ThrowExceptionToJS(isolate, throwable);
                    return;
                }

                info.GetReturnValue().Set(Accessor::wrapAsObject(isolate, context, javaClassObject,
                                                                 env->CallObjectMethod(
                                                                         javaClassObject,
                                                                         getClass)));
            }).ToLocalChecked()
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "__createJavaObject"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                JNIEnv *env = Main::env();
                v8::Local<v8::String> className = info[0].As<v8::String>();
                v8::Local<v8::Array> args = info[1].As<v8::Array>();
                jclass javaBridgeUtilClass = env->FindClass(
                        "com/mucheng/nodejava/javabridge/JavaBridgeUtil");
                jclass objectClass = env->FindClass("java/lang/Object");
                jclass javaDoubleClass = env->FindClass("java/lang/Double");
                jclass javaBooleanClass = env->FindClass("java/lang/Boolean");
                jmethodID doubleValueOf = env->GetStaticMethodID(javaDoubleClass, "valueOf",
                                                                 "(D)Ljava/lang/Double;");
                jmethodID booleanValueOf = env->GetStaticMethodID(javaBooleanClass, "valueOf",
                                                                  "(Z)Ljava/lang/Boolean;");
                jmethodID newInstance = env->GetStaticMethodID(javaBridgeUtilClass, "newInstance",
                                                               "(Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/Object;");

                int length = args->Length();
                jobjectArray params = env->NewObjectArray(length, objectClass, nullptr);
                for (int index = 0; index < length; index++) {
                    v8::Local<v8::Value> value = args->Get(context, index).ToLocalChecked();
                    if (value->IsNumber()) {
                        env->SetObjectArrayElement(params, index,
                                                   env->CallStaticObjectMethod(javaDoubleClass,
                                                                               doubleValueOf,
                                                                               value.As<v8::Number>()->Value()));
                    } else if (value->IsString()) {
                        env->SetObjectArrayElement(params, index, Util::CStr2JavaStr(
                                *v8::String::Utf8Value(isolate, value.As<v8::String>())));
                    } else if (value->IsBoolean()) {
                        env->SetObjectArrayElement(params, index,
                                                   env->CallStaticObjectMethod(javaBooleanClass,
                                                                               booleanValueOf,
                                                                               value.As<v8::Boolean>()->Value()));
                    } else if (value->IsObject()) {
                        v8::Local<v8::Object> obj = value.As<v8::Object>();
                        v8::MaybeLocal<v8::Value> maybeExternal = obj->Get(context,
                                                                           v8::Symbol::For(isolate,
                                                                                           v8::String::NewFromUtf8Literal(
                                                                                                   isolate,
                                                                                                   "__ref__")));
                        if (!maybeExternal.IsEmpty() &&
                            maybeExternal.ToLocalChecked()->IsExternal()) {
                            env->SetObjectArrayElement(params, index,
                                                       static_cast<jobject>(maybeExternal.ToLocalChecked().As<v8::External>()->Value())
                            );
                        } else {
                            isolate->ThrowException(v8::Exception::TypeError(
                                    v8::String::NewFromUtf8Literal(isolate, "Unknown type")));
                            return;
                        }
                    }
                }

                jobject temp = env->CallStaticObjectMethod(javaBridgeUtilClass, newInstance,
                                                           Util::CStr2JavaStr(
                                                                   *v8::String::Utf8Value(isolate,
                                                                                          className)),
                                                           params);

                if (env->ExceptionCheck()) {
                    jthrowable throwable = env->ExceptionOccurred();
                    env->ExceptionClear();
                    Util::ThrowExceptionToJS(isolate, throwable);
                    return;
                }

                info.GetReturnValue().Set(v8::External::New(isolate, env->NewGlobalRef(temp)));
            }).ToLocalChecked()
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "__makeReference"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                v8::Local<v8::Object> target = info[0].As<v8::Object>();
                v8::Local<v8::External> ref = info[1].As<v8::External>();
                target->Set(context, v8::Symbol::For(isolate,
                                                     v8::String::NewFromUtf8Literal(isolate,
                                                                                    "__ref__")),
                            ref).Check();
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
