#include "embedding.h"
#include "main.h"
#include "log.h"
#include "javabridge/ClassInfo.h"
#include "javabridge/Wrapper.h"
#include "Util.h"
#include "uv.h"
#include <queue>
#include <mutex>
#include <unistd.h>
#include <memory>

v8::Local<v8::Value> makeJSReturnValue(
        v8::Isolate *isolate,
        jobject javaObject,
        bool isVoid
);

jobject makeJavaReturnValue(
        v8::Isolate *isolate,
        v8::Local<v8::Value> jsObject
);

class Interface {
public:
    v8::Isolate *isolate;
    v8::Persistent<v8::Value> value;
    pthread_t threadId;

    Interface(
            v8::Isolate *isolate,
            v8::Local<v8::Value> value,
            pthread_t threadId
    );

    static void To(jobject instance, Interface *self);

    static Interface *From(jobject instance);
};

Interface::Interface(
        v8::Isolate *isolate,
        v8::Local<v8::Value> value,
        pthread_t threadId
) {
    this->isolate = isolate;
    this->value.Reset(isolate, value);
    this->threadId = threadId;
}

void Interface::To(jobject instance, Interface *self) {
    Util::SetPtr(instance, "interfacePtr", self);
}

Interface *Interface::From(jobject instance) {
    return Util::GetPtrAs<Interface *>(instance, "interfacePtr");
}

pthread_t mainThreadId;
pthread_t eventLoopThreadId;

bool threadIdEquals(pthread_t a, pthread_t b) {
    return pthread_equal(a, b);
}

v8::Local<v8::Value> getClassInfo(
        v8::Isolate *isolate,
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

    return ClassInfo::BuildObject(isolate, classInfoInstance);
}

v8::Local<v8::Value> classForName(v8::Isolate *isolate, v8::Local<v8::String> className) {
    JNIEnv *env = Main::env();
    jclass javaBridgeUtilClass = env->FindClass("com/mucheng/nodejava/javabridge/JavaBridgeUtil");
    jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
    jmethodID getClassLoader = env->GetStaticMethodID(javaBridgeUtilClass, "getClassLoader",
                                                      "()Ljava/lang/ClassLoader;");
    jmethodID loadClass = env->GetMethodID(classLoaderClass, "loadClass",
                                           "(Ljava/lang/String;)Ljava/lang/Class;");
    jobject classLoaderObject = env->CallStaticObjectMethod(javaBridgeUtilClass, getClassLoader);
    jobject javaClassObject = env->CallObjectMethod(classLoaderObject, loadClass,
                                                    Util::CStr2JavaStr(
                                                            *v8::String::Utf8Value(isolate,
                                                                                   className)));

    if (env->ExceptionCheck()) {
        jthrowable throwable = env->ExceptionOccurred();
        env->ExceptionClear();
        Util::ThrowExceptionToJS(isolate, throwable);
        return v8::Null(isolate);
    }

    return makeJSReturnValue(isolate, javaClassObject, false);
}

v8::Local<v8::Value> makeJSReturnValue(
        v8::Isolate *isolate,
        jobject javaObject,
        bool isVoid
) {
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    JNIEnv *env = Main::env();
    jclass javaObjectClass = env->FindClass("java/lang/Object");
    jclass javaClazz = env->FindClass("java/lang/Class");
    jclass arrayClass = env->FindClass("java/lang/reflect/Array");
    jclass numberClass = env->FindClass("java/lang/Number");
    jclass booleanClass = env->FindClass("java/lang/Boolean");
    jclass charactorClass = env->FindClass("java/lang/Character");

    jmethodID getClass = env->GetMethodID(javaObjectClass, "getClass", "()Ljava/lang/Class;");
    jmethodID getName = env->GetMethodID(javaClazz, "getName", "()Ljava/lang/String;");
    jmethodID isArray = env->GetMethodID(javaClazz, "isArray", "()Z");
    jmethodID get = env->GetStaticMethodID(arrayClass, "get",
                                           "(Ljava/lang/Object;I)Ljava/lang/Object;");
    jmethodID byteValue = env->GetMethodID(numberClass, "byteValue", "()B");
    jmethodID shortValue = env->GetMethodID(numberClass, "shortValue", "()S");
    jmethodID intValue = env->GetMethodID(numberClass, "intValue", "()I");
    jmethodID longValue = env->GetMethodID(numberClass, "longValue", "()J");
    jmethodID floatValue = env->GetMethodID(numberClass, "floatValue", "()F");
    jmethodID doubleValue = env->GetMethodID(numberClass, "doubleValue", "()D");
    jmethodID booleanValue = env->GetMethodID(booleanClass, "booleanValue", "()Z");
    jmethodID toString = env->GetMethodID(charactorClass, "toString", "()Ljava/lang/String;");

    v8::Local<v8::Value> javaClass;
    v8::Local<v8::Value> value;

    jobject javaObjectClazz =
            javaObject != nullptr ? env->CallObjectMethod(javaObject, getClass) : nullptr;
    const char *typeStr = javaObjectClazz != nullptr ? Util::JavaStr2CStr(
            static_cast<jstring>(env->CallObjectMethod(javaObjectClazz, getName))) : nullptr;

    if (typeStr == nullptr) {
        javaClass = v8::Null(isolate);
        if (isVoid) {
            value = v8::Undefined(isolate);
        } else {
            value = v8::Null(isolate);
        }
    } else if (!strcmp(typeStr, "java.lang.Void")) {
        javaClass = v8::Null(isolate);
        value = v8::Undefined(isolate);
    } else if (!strcmp(typeStr, "java.lang.Byte")) {
        javaClass = v8::Null(isolate);
        value = v8::Integer::New(isolate, env->CallByteMethod(javaObject, byteValue));
    } else if (!strcmp(typeStr, "java.lang.Short")) {
        javaClass = v8::Null(isolate);
        value = v8::Integer::New(isolate, env->CallShortMethod(javaObject, shortValue));
    } else if (!strcmp(typeStr, "java.lang.Integer")) {
        javaClass = v8::Null(isolate);
        value = v8::Integer::New(isolate, env->CallIntMethod(javaObject, intValue));
    } else if (!strcmp(typeStr, "java.lang.Long")) {
        javaClass = v8::Null(isolate);
        value = v8::BigInt::New(isolate, env->CallLongMethod(javaObject, longValue));
    } else if (!strcmp(typeStr, "java.lang.Float")) {
        javaClass = v8::Null(isolate);
        value = v8::Number::New(isolate, env->CallFloatMethod(javaObject, floatValue));
    } else if (!strcmp(typeStr, "java.lang.Double")) {
        javaClass = v8::Null(isolate);
        value = v8::Number::New(isolate, env->CallDoubleMethod(javaObject, doubleValue));
    } else if (!strcmp(typeStr, "java.lang.Boolean")) {
        javaClass = v8::Null(isolate);
        value = v8::Boolean::New(isolate, env->CallBooleanMethod(javaObject, booleanValue));
    } else if (!strcmp(typeStr, "java.lang.Charactor")) {
        javaClass = v8::Null(isolate);
        value = v8::String::NewFromUtf8(isolate, Util::JavaStr2CStr(
                static_cast<jstring>(env->CallObjectMethod(javaObject,
                                                           toString)))).ToLocalChecked();
    } else if (!strcmp(typeStr, "java.lang.String")) {
        javaClass = v8::Null(isolate);
        value = v8::String::NewFromUtf8(isolate, Util::JavaStr2CStr(
                static_cast<jstring>(javaObject))).ToLocalChecked();
    } else if (env->CallBooleanMethod(javaObjectClazz, isArray)) {
        jobjectArray javaObjectArray = static_cast<jobjectArray>(javaObject);
        int length = env->GetArrayLength(javaObjectArray);
        javaClass = v8::String::NewFromUtf8Literal(isolate, "array");
        v8::Local<v8::Array> array = v8::Array::New(isolate, length);
        value = array;

        for (int index = 0; index < length; index++) {
            jobject element = env->CallStaticObjectMethod(arrayClass, get, javaObjectArray, index);
            array->Set(context, index,
                       makeJSReturnValue(isolate, element, false)).Check();
        }
    } else {
        javaClass = v8::String::NewFromUtf8(isolate, typeStr).ToLocalChecked();
        value = Util::NewExternal(isolate, env->NewGlobalRef(javaObject));
    }

    v8::Local<v8::Object> result = v8::Object::New(isolate);
    result->Set(context, v8::String::NewFromUtf8Literal(isolate, "javaClass"),
                javaClass).Check();
    result->Set(context, v8::String::NewFromUtf8Literal(isolate, "value"), value).Check();
    return result;
}

jobject makeJavaReturnValue(
        v8::Isolate *isolate,
        v8::Local<v8::Value> jsObject
) {
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    JNIEnv *env = Main::env();
    jclass javaObjectClass = env->FindClass("java/lang/Object");
    jclass integerClass = env->FindClass("java/lang/Integer");
    jclass longClass = env->FindClass("java/lang/Long");
    jclass doubleClass = env->FindClass("java/lang/Double");
    jclass booleanClass = env->FindClass("java/lang/Boolean");
    jclass interfaceClass = env->FindClass("com/mucheng/nodejava/javabridge/Interface");
    jmethodID integerValueOf = env->GetStaticMethodID(integerClass, "valueOf",
                                                      "(I)Ljava/lang/Integer;");
    jmethodID longValueOf = env->GetStaticMethodID(longClass, "valueOf", "(J)Ljava/lang/Long;");
    jmethodID doubleValueOf = env->GetStaticMethodID(doubleClass, "valueOf",
                                                     "(D)Ljava/lang/Double;");
    jmethodID booleanValueOf = env->GetStaticMethodID(booleanClass, "valueOf",
                                                      "(Z)Ljava/lang/Boolean;");

    if (jsObject->IsNull() || jsObject->IsUndefined()) {
        return nullptr;
    } else if (jsObject->IsInt32()) {
        return env->CallStaticObjectMethod(integerClass, integerValueOf,
                                           jsObject.As<v8::Int32>()->Value());
    } else if (jsObject->IsBigInt()) {
        return env->CallStaticObjectMethod(longClass, longValueOf,
                                           jsObject.As<v8::BigInt>()->Int64Value());
    } else if (jsObject->IsNumber()) {
        return env->CallStaticObjectMethod(doubleClass, doubleValueOf,
                                           jsObject.As<v8::Number>()->Value());
    } else if (jsObject->IsBoolean()) {
        return env->CallStaticObjectMethod(booleanClass, booleanValueOf,
                                           jsObject.As<v8::Boolean>()->Value());
    } else if (jsObject->IsString()) {
        return Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, jsObject.As<v8::String>()));
    } else if (jsObject->IsArray()) {
        v8::Local<v8::Array> array = jsObject.As<v8::Array>();
        int length = array->Length();
        jobjectArray javaArray = env->NewObjectArray(length, javaObjectClass, nullptr);
        for (int index = 0; index < length; index++) {
            v8::Local<v8::Value> element = array->Get(context, index).ToLocalChecked();
            jobject javaReturnValue = makeJavaReturnValue(isolate, element);
            env->SetObjectArrayElement(javaArray, index, javaReturnValue);
        }
    } else if (jsObject->IsObject() &&
               Wrapper::IsWrapped(isolate, context, jsObject.As<v8::Object>())) {
        return Wrapper::Unwrap(isolate, context, jsObject.As<v8::Object>());
    } else if (jsObject->IsObject()) {
        jobject interfaceObject = env->AllocObject(interfaceClass);
        Interface *interface = new Interface(isolate, jsObject, pthread_self());
        Interface::To(interfaceObject, interface);
        return interfaceObject;
    }

    return nullptr;
}

v8::Local<v8::Value> getField(
        v8::Isolate *isolate,
        v8::Local<v8::String> className,
        v8::Local<v8::String> fieldName,
        v8::Local<v8::Object> self
) {
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    JNIEnv *env = Main::env();
    jclass javaBridgeUtil = env->FindClass("com/mucheng/nodejava/javabridge/JavaBridgeUtil");
    jmethodID getField = env->GetStaticMethodID(javaBridgeUtil, "getField",
                                                "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/Object;)Ljava/lang/Object;");

    jobject javaObject;
    if (!Wrapper::IsWrapped(isolate, context, self)) {
        javaObject = env->CallStaticObjectMethod(
                javaBridgeUtil,
                getField,
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, className)),
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, fieldName)),
                nullptr
        );
    } else {
        javaObject = env->CallStaticObjectMethod(
                javaBridgeUtil,
                getField,
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, className)),
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, fieldName)),
                Wrapper::Unwrap(isolate, context, self)
        );
    }

    if (env->ExceptionCheck()) {
        jthrowable throwable = env->ExceptionOccurred();
        env->ExceptionClear();
        Util::ThrowExceptionToJS(isolate, throwable);
        return v8::Null(isolate);
    }

    return makeJSReturnValue(
            isolate,
            javaObject,
            false
    );
}

void setField(
        v8::Isolate *isolate,
        v8::Local<v8::String> className,
        v8::Local<v8::String> fieldName,
        v8::Local<v8::Value> value,
        v8::Local<v8::Object> self
) {
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    JNIEnv *env = Main::env();
    jclass javaBridgeUtil = env->FindClass("com/mucheng/nodejava/javabridge/JavaBridgeUtil");
    jmethodID callMethod = env->GetStaticMethodID(javaBridgeUtil, "setField",
                                                  "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/Object;Ljava/lang/Object;)V");
    jobject javaObject = makeJavaReturnValue(isolate, value);

    if (!Wrapper::IsWrapped(isolate, context, self)) {
        env->CallStaticVoidMethod(
                javaBridgeUtil,
                callMethod,
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, className)),
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, fieldName)),
                javaObject,
                nullptr
        );
    } else {
        env->CallStaticVoidMethod(
                javaBridgeUtil,
                callMethod,
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, className)),
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, fieldName)),
                javaObject,
                Wrapper::Unwrap(isolate, context, self)
        );
    }

    if (env->ExceptionCheck()) {
        jthrowable throwable = env->ExceptionOccurred();
        env->ExceptionClear();
        Util::ThrowExceptionToJS(isolate, throwable);
    }
}

v8::Local<v8::Value> callMethod(
        v8::Isolate *isolate,
        v8::Local<v8::String> className,
        v8::Local<v8::String> methodName,
        v8::Local<v8::Array> arguments,
        v8::Local<v8::Object> self
) {
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    JNIEnv *env = Main::env();
    jclass javaBridgeUtil = env->FindClass("com/mucheng/nodejava/javabridge/JavaBridgeUtil");
    jclass javaObjectClass = env->FindClass("java/lang/Object");
    jclass booleanClass = env->FindClass("java/lang/Boolean");
    jmethodID callMethod = env->GetStaticMethodID(javaBridgeUtil, "callMethod",
                                                  "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/Object;Ljava/lang/Object;)[Ljava/lang/Object;");
    jmethodID booleanValue = env->GetMethodID(booleanClass, "booleanValue", "()Z");

    int length = arguments->Length();
    jobjectArray javaParams = env->NewObjectArray(length, javaObjectClass, nullptr);
    for (int index = 0; index < length; index++) {
        v8::Local<v8::Value> jsObject = arguments->Get(context, index).ToLocalChecked();
        jobject javaReturnValue = makeJavaReturnValue(isolate, jsObject);
        env->SetObjectArrayElement(javaParams, index, javaReturnValue);
    }

    jobjectArray javaObjectArray;
    if (!Wrapper::IsWrapped(isolate, context, self)) {
        javaObjectArray = static_cast<jobjectArray>(env->CallStaticObjectMethod(
                javaBridgeUtil,
                callMethod,
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, className)),
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, methodName)),
                javaParams,
                nullptr
        ));
    } else {
        javaObjectArray = static_cast<jobjectArray>(env->CallStaticObjectMethod(
                javaBridgeUtil,
                callMethod,
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, className)),
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, methodName)),
                javaParams,
                Wrapper::Unwrap(isolate, context, self)
        ));
    }

    if (env->ExceptionCheck()) {
        jthrowable throwable = env->ExceptionOccurred();
        env->ExceptionClear();
        Util::ThrowExceptionToJS(isolate, throwable);
        return v8::Null(isolate);
    }

    return makeJSReturnValue(
            isolate,
            env->GetObjectArrayElement(javaObjectArray, 0),
            env->CallBooleanMethod(env->GetObjectArrayElement(javaObjectArray, 1), booleanValue)
    );
}

void makeReference(
        v8::Isolate *isolate,
        v8::Local<v8::Object> target,
        v8::Local<v8::External> value
) {
    Wrapper::WrapTo(isolate, isolate->GetCurrentContext(), target, static_cast<jobject>(value->Value()));
}

void defineClass(
        v8::Isolate *isolate,
        v8::Local<v8::Context> context,
        v8::Local<v8::String> className,
        v8::Local<v8::String> superclass,
        v8::Local<v8::Array> implementations,
        v8::Local<v8::Array> methods,
        v8::Local<v8::String> outputDexFile
);

void defineClass(
        v8::Isolate *isolate,
        v8::Local<v8::String> className,
        v8::Local<v8::String> superclass,
        v8::Local<v8::Array> implementations,
        v8::Local<v8::Array> methods,
        v8::Local<v8::String> outputDexFile
) {
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    JNIEnv *env = Main::env();
    jclass javaBridgeUtilClass = env->FindClass("com/mucheng/nodejava/javabridge/JavaBridgeUtil");
    jclass stringClass = env->FindClass("java/lang/String");
    jmethodID defineClass = env->GetStaticMethodID(javaBridgeUtilClass, "defineClass",
                                                   "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)V");
    jobject javaClassName = Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, className));
    jobject javaSuperclass = Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, superclass));

    int implementationLength = implementations->Length();
    jobjectArray javaImplementations = env->NewObjectArray(implementationLength, stringClass,
                                                           nullptr);
    for (int index = 0; index < implementationLength; index++) {
        jobject implementation = Util::CStr2JavaStr(*v8::String::Utf8Value(isolate,
                                                                           implementations->Get(
                                                                                   context,
                                                                                   index).ToLocalChecked().As<v8::String>()));
        env->SetObjectArrayElement(javaImplementations, index, implementation);
    }

    int methodLength = methods->Length();
    jobjectArray javaMethods = env->NewObjectArray(methodLength, stringClass,
                                                   nullptr);
    for (int index = 0; index < methodLength; index++) {
        jobject method = Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, methods->Get(context,
                                                                                         index).ToLocalChecked().As<v8::String>()));
        env->SetObjectArrayElement(javaMethods, index, method);
    }

    jobject javaOutputDexFile = Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, outputDexFile));
    env->CallStaticVoidMethod(
            javaBridgeUtilClass, defineClass,
            javaClassName, javaSuperclass, javaImplementations, javaMethods, javaOutputDexFile
    );
    if (env->ExceptionCheck()) {
        jthrowable throwable = env->ExceptionOccurred();
        env->ExceptionClear();
        Util::ThrowExceptionToJS(isolate, throwable);
        return;
    }
};

v8::Local<v8::Value> createJavaObject(
        v8::Isolate *isolate,
        v8::Local<v8::String> className,
        v8::Local<v8::Array> arguments
) {
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    JNIEnv *env = Main::env();
    jclass javaBridgeUtilClass = env->FindClass("com/mucheng/nodejava/javabridge/JavaBridgeUtil");
    jclass javaObjectClass = env->FindClass("java/lang/Object");
    jmethodID getConstructor = env->GetStaticMethodID(javaBridgeUtilClass, "getConstructor",
                                                      "(Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/Object;");

    int length = arguments->Length();
    jobjectArray javaParams = env->NewObjectArray(length, javaObjectClass, nullptr);

    for (int index = 0; index < length; index++) {
        v8::Local<v8::Value> jsObject = arguments->Get(context, index).ToLocalChecked();
        jobject javaReturnValue = makeJavaReturnValue(isolate, jsObject);
        env->SetObjectArrayElement(javaParams, index, javaReturnValue);
    }

    jobject javaObject = env->CallStaticObjectMethod(javaBridgeUtilClass, getConstructor,
                                                     Util::CStr2JavaStr(
                                                             *v8::String::Utf8Value(isolate,
                                                                                    className)),
                                                     javaParams);

    if (env->ExceptionCheck()) {
        jthrowable throwable = env->ExceptionOccurred();
        env->ExceptionClear();
        Util::ThrowExceptionToJS(isolate, throwable);
        return v8::Null(isolate);
    }

    return v8::External::New(isolate, env->NewGlobalRef(javaObject));
}

class AsyncOrSyncCallback {
public:
    Interface *interface;
    jstring methodNameStr;
    jobjectArray params;
    jobject result;
    bool done;

    AsyncOrSyncCallback(Interface *interface, jstring methodNameStr, jobjectArray params);
};

AsyncOrSyncCallback::AsyncOrSyncCallback(Interface *interface, jstring methodNameStr,
                                         jobjectArray params) {
    this->interface = interface;
    this->methodNameStr = methodNameStr;
    this->params = params;
}

std::queue<AsyncOrSyncCallback *> queue;
uv_mutex_t uvMutexTask;
uv_async_t uvAsyncTask;

void asyncOrSyncCall(AsyncOrSyncCallback *asyncCall, bool crossThread) {
    Interface *interface = asyncCall->interface;
    jstring methodNameStr = asyncCall->methodNameStr;
    jobjectArray params = asyncCall->params;

    v8::Isolate *isolate = interface->isolate;
    v8::Locker locker(isolate);
    v8::HandleScope handleScope(isolate);

    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    JNIEnv *env = Main::env();

    if (methodNameStr == nullptr) {
        v8::Local<v8::Function> value = interface->value.Get(isolate).As<v8::Function>();
        int length = env->GetArrayLength(params);
        v8::Local<v8::Value> jsParams[length];
        for (int index = 0; index < length; index++) {
            jsParams[index] = makeJSReturnValue(isolate,
                                                env->GetObjectArrayElement(params, index),
                                                false);
        }

        v8::MaybeLocal<v8::Value> result = value->Call(context, context->Global(), length,
                                                       jsParams);
        if (result.IsEmpty()) {
            asyncCall->result = nullptr;
            asyncCall->done = true;
            return;
        }

        jobject javaResult = makeJavaReturnValue(isolate, result.ToLocalChecked());
        if (crossThread) {
            javaResult = env->NewGlobalRef(javaResult);
        }
        asyncCall->result = javaResult;
        asyncCall->done = true;
        return;
    } else {
        v8::Local<v8::Object> value = interface->value.Get(isolate).As<v8::Object>();
        v8::Local<v8::String> methodName = v8::String::NewFromUtf8(isolate, Util::JavaStr2CStr(
                methodNameStr)).ToLocalChecked();

        v8::Local<v8::Function> fn = value->Get(context,
                                                methodName).ToLocalChecked().As<v8::Function>();
        int length = env->GetArrayLength(params);
        v8::Local<v8::Value> jsParams[length];
        for (int index = 0; index < length; index++) {
            jsParams[index] = makeJSReturnValue(isolate,
                                                env->GetObjectArrayElement(params, index),
                                                false);
        }

        v8::MaybeLocal<v8::Value> result = fn->Call(context, context->Global(), length,
                                                    jsParams);
        if (result.IsEmpty()) {
            asyncCall->result = nullptr;
            asyncCall->done = true;
            return;
        }

        jobject javaResult = makeJavaReturnValue(isolate, result.ToLocalChecked());
        if (crossThread) {
            javaResult = env->NewGlobalRef(javaResult);
        }
        asyncCall->result = javaResult;
        asyncCall->done = true;
        return;
    }
}

void cb_asyncOrSyncCall(uv_async_t *handle) {
    AsyncOrSyncCallback *callData;
    eventLoopThreadId = pthread_self();
    do {
        uv_mutex_lock(&uvMutexTask);
        if (!queue.empty()) {
            callData = queue.front();
            queue.pop();
        } else {
            callData = nullptr;
        }
        uv_mutex_unlock(&uvMutexTask);

        if (callData) {
            asyncOrSyncCall(callData, true);
        }
    } while (callData);
}

void JAVA_ACCESSOR_BINDING(
        v8::Local<v8::Object> exports,
        v8::Local<v8::Value>,
        v8::Local<v8::Context> context,
        void *priv
) {
    v8::Isolate *isolate = context->GetIsolate();
    mainThreadId = pthread_self();

    uv_mutex_init(&uvMutexTask);
    uv_async_init(node::GetCurrentEventLoop(isolate), &uvAsyncTask,
                  cb_asyncOrSyncCall);

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "className"),
            v8::Symbol::For(isolate, v8::String::NewFromUtf8Literal(isolate, "className"))
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "__classForName"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                info.GetReturnValue().Set(classForName(isolate, info[0].As<v8::String>()));
            }).ToLocalChecked()
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "getClassInfo"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                info.GetReturnValue().Set(getClassInfo(isolate, info[0].As<v8::String>()));
            }).ToLocalChecked()
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "__getField"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                info.GetReturnValue().Set(getField(isolate, info[0].As<v8::String>(),
                                                   info[1].As<v8::String>(),
                                                   info[2].As<v8::Object>()));
            }).ToLocalChecked()
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "__setField"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                setField(isolate, info[0].As<v8::String>(),
                         info[1].As<v8::String>(),
                         info[2].As<v8::Value>(),
                         info[2].As<v8::Object>());
            }).ToLocalChecked()
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "__callMethod"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                info.GetReturnValue().Set(callMethod(isolate, info[0].As<v8::String>(),
                                                     info[1].As<v8::String>(),
                                                     info[2].As<v8::Array>(),
                                                     info[3].As<v8::Object>()));
            }).ToLocalChecked()
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "__createJavaObject"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                info.GetReturnValue().Set(
                        createJavaObject(isolate, info[0].As<v8::String>(),
                                         info[1].As<v8::Array>()));
            }).ToLocalChecked()
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "__makeReference"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                makeReference(isolate, info[0].As<v8::Object>(),
                              info[1].As<v8::External>());
            }).ToLocalChecked()
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "__defineClass"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                v8::Local<v8::String> className = info[0].As<v8::String>();
                v8::Local<v8::String> superclass = info[1].As<v8::String>();
                v8::Local<v8::Array> implementations = info[2].As<v8::Array>();
                v8::Local<v8::Array> methods = info[3].As<v8::Array>();
                v8::Local<v8::String> outputDexFile = info[4].As<v8::String>();
                defineClass(isolate, className, superclass, implementations,
                            methods,
                            outputDexFile);
            }).ToLocalChecked()
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "__loadDex"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                v8::Local<v8::String> dex = info[0].As<v8::String>();
                JNIEnv *env = Main::env();
                jclass javaBridgeUtilClass = env->FindClass(
                        "com/mucheng/nodejava/javabridge/JavaBridgeUtil");
                jmethodID loadDex = env->GetStaticMethodID(javaBridgeUtilClass, "loadDex",
                                                           "(Ljava/lang/String;)V");
                env->CallStaticVoidMethod(javaBridgeUtilClass, loadDex,
                                          Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, dex)));
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

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_mucheng_nodejava_javabridge_Interface_nativeIsFunction(JNIEnv *env, jobject thiz) {
    Interface *interface = Interface::From(thiz);
    v8::Isolate *isolate = interface->isolate;
    return interface->value.Get(isolate)->IsFunction();
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_mucheng_nodejava_javabridge_Interface_nativeInvoke__Ljava_lang_String_2_3Ljava_lang_Object_2(
        JNIEnv *env, jobject thiz, jstring methodNameStr, jobjectArray params) {
    Interface *interface = Interface::From(thiz);
    if (threadIdEquals(mainThreadId, pthread_self())) {
        AsyncOrSyncCallback call = AsyncOrSyncCallback(interface, methodNameStr, params);
        asyncOrSyncCall(&call, false);
        return call.result;
    } else {
        LOGE("Exec");
        AsyncOrSyncCallback call = AsyncOrSyncCallback(
                interface,
                (jstring) (methodNameStr != nullptr ? env->NewGlobalRef(methodNameStr)
                                                    : methodNameStr),
                (jobjectArray) (params != nullptr ? env->NewGlobalRef(params) : params)
        );
        uv_mutex_lock(&uvMutexTask);
        queue.push(&call);
        uv_mutex_unlock(&uvMutexTask);
        uv_async_send(&uvAsyncTask);
        while (!call.done);
        return call.result;
    }
}