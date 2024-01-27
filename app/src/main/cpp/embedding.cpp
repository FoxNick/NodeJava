#include "embedding.h"
#include "main.h"
#include "log.h"
#include "javabridge/ClassInfo.h"
#include "javabridge//Wrapper.h"
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

v8::Local<v8::Value> makeReturnValue(
        v8::Isolate *isolate,
        v8::Local<v8::Context> context,
        jobject type,
        jobject componentType,
        jobject javaObject
) {
    JNIEnv *env = Main::env();
    jclass javaObjectClass = env->FindClass("java/lang/Object");
    jclass javaClazz = env->FindClass("java/lang/Class");
    jclass arrayClass = env->FindClass("java/lang/reflect/Array");
    jclass numberClass = env->FindClass("java/lang/Number");
    jclass booleanClass = env->FindClass("java/lang/Boolean");
    jclass charactorClass = env->FindClass("java/lang/Character");
    jmethodID getComponentType = env->GetMethodID(javaClazz, "getComponentType",
                                                  "()Ljava/lang/Class;");
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

    const char *typeStr = Util::JavaStr2CStr(
            static_cast<jstring>(env->CallObjectMethod(type, getName)));

    if (!strcmp(typeStr, "byte")) {
        javaClass = v8::Null(isolate);
        value = v8::Integer::New(isolate, env->CallByteMethod(javaObject, byteValue));
    } else if (!strcmp(typeStr, "short")) {
        javaClass = v8::Null(isolate);
        value = v8::Integer::New(isolate, env->CallShortMethod(javaObject, shortValue));
    } else if (!strcmp(typeStr, "int")) {
        javaClass = v8::Null(isolate);
        value = v8::Integer::New(isolate, env->CallIntMethod(javaObject, intValue));
    } else if (!strcmp(typeStr, "long")) {
        javaClass = v8::Null(isolate);
        value = v8::BigInt::New(isolate, env->CallLongMethod(javaObject, longValue));
    } else if (!strcmp(typeStr, "float")) {
        javaClass = v8::Null(isolate);
        value = v8::Number::New(isolate, env->CallFloatMethod(javaObject, floatValue));
    } else if (!strcmp(typeStr, "double")) {
        javaClass = v8::Null(isolate);
        value = v8::Number::New(isolate, env->CallDoubleMethod(javaObject, doubleValue));
    } else if (!strcmp(typeStr, "boolean")) {
        javaClass = v8::Null(isolate);
        value = v8::Boolean::New(isolate, env->CallBooleanMethod(javaObject, booleanValue));
    } else if (!strcmp(typeStr, "char")) {
        javaClass = v8::Null(isolate);
        value = v8::String::NewFromUtf8(isolate, Util::JavaStr2CStr(
                static_cast<jstring>(env->CallObjectMethod(javaObject,
                                                           toString)))).ToLocalChecked();
    } else if (!strcmp(typeStr, "java.lang.String")) {
        javaClass = v8::Null(isolate);
        value = v8::String::NewFromUtf8(isolate, Util::JavaStr2CStr(
                static_cast<jstring>(javaObject))).ToLocalChecked();
    } else if (env->CallBooleanMethod(type, isArray)) {
        jobjectArray javaObjectArray = static_cast<jobjectArray>(javaObject);
        int length = env->GetArrayLength(javaObjectArray);
        javaClass = v8::String::NewFromUtf8Literal(isolate, "array");
        v8::Local<v8::Array> array = v8::Array::New(isolate, length);
        value = array;

        for (int index = 0; index < length; index++) {
            jobject element = env->CallStaticObjectMethod(arrayClass, get, javaObjectArray, index);

            jobject elementClass = env->CallObjectMethod(element, getClass);
            jobject elementComponentType = env->CallObjectMethod(elementClass, getComponentType);
            array->Set(context, index,
                       makeReturnValue(isolate, context, componentType, elementComponentType,
                                       element)).Check();
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

v8::Local<v8::Value> getField(
        v8::Isolate *isolate,
        v8::Local<v8::Context> context,
        v8::Local<v8::String> className,
        v8::Local<v8::String> fieldName,
        v8::Local<v8::Object> self
) {
    JNIEnv *env = Main::env();
    jclass javaBridgeUtil = env->FindClass("com/mucheng/nodejava/javabridge/JavaBridgeUtil");
    jmethodID getField = env->GetStaticMethodID(javaBridgeUtil, "getField",
                                                "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/Object;)[Ljava/lang/Object;");

    jobjectArray javaObjectArray;
    if (!Wrapper::IsWrapped(isolate, context, self)) {
        javaObjectArray = static_cast<jobjectArray>(env->CallStaticObjectMethod(
                javaBridgeUtil,
                getField,
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, className)),
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, fieldName)),
                nullptr
        ));
    } else {
        javaObjectArray = static_cast<jobjectArray>(env->CallStaticObjectMethod(
                javaBridgeUtil,
                getField,
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, className)),
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, fieldName)),
                Wrapper::Unwrap(isolate, context, self)
        ));
    }

    if (env->ExceptionCheck()) {
        jthrowable throwable = env->ExceptionOccurred();
        env->ExceptionClear();
        Util::ThrowExceptionToJS(isolate, throwable);
        return v8::Null(isolate);
    }

    return makeReturnValue(
            isolate,
            context,
            env->GetObjectArrayElement(javaObjectArray, 0),
            env->GetObjectArrayElement(javaObjectArray, 1),
            env->GetObjectArrayElement(javaObjectArray, 2)
    );
}

v8::Local<v8::Value> callMethod(
        v8::Isolate *isolate,
        v8::Local<v8::Context> context,
        v8::Local<v8::String> className,
        v8::Local<v8::String> fieldName,
        v8::Local<v8::Array> arguments,
        v8::Local<v8::Object> self
) {
    return v8::Local<v8::Value>();
}

void makeReference(
        v8::Isolate *isolate,
        v8::Local<v8::Context> context,
        v8::Local<v8::Object> target,
        v8::Local<v8::External> value
) {
    Wrapper::WrapTo(isolate, context, target, static_cast<jobject>(value->Value()));
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
            v8::String::NewFromUtf8Literal(isolate, "__getField"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                info.GetReturnValue().Set(getField(isolate, context, info[0].As<v8::String>(),
                                                   info[1].As<v8::String>(),
                                                   info[2].As<v8::Object>()));
            }).ToLocalChecked()
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "__callMethod"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                info.GetReturnValue().Set(callMethod(isolate, context, info[0].As<v8::String>(),
                                                     info[1].As<v8::String>(),
                                                     info[2].As<v8::Array>(),
                                                     info[3].As<v8::Object>()));
            }).ToLocalChecked()
    ).Check();

    exports->Set(
            context,
            v8::String::NewFromUtf8Literal(isolate, "__makeReference"),
            v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value> &info) {
                SETUP_CALLBACK_INFO();
                makeReference(isolate, context, info[0].As<v8::Object>(),
                              info[1].As<v8::External>());
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
