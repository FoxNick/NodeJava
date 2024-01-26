#include "Accessor.h"
#include "main.h"
#include "Util.h"
#include "log.h"
#include <node.h>

v8::Local<v8::Object> Accessor::wrapAsObject(
        v8::Isolate *isolate,
        v8::Local<v8::Context> context,
        jobject javaObject,
        jobject typeClass
) {
    v8::Local<v8::Object> result = v8::Object::New(isolate);
    JNIEnv *env = Main::env();

    jclass javaClass = env->FindClass("java/lang/Class");
    jclass javaBridgeUtilClass = env->FindClass("com/mucheng/nodejava/javabridge/JavaBridgeUtil");

    jmethodID asNumber = env->GetStaticMethodID(javaBridgeUtilClass, "asNumber",
                                                "(Ljava/lang/Object;)D");
    jmethodID asString = env->GetStaticMethodID(javaBridgeUtilClass, "asString",
                                                "(Ljava/lang/Object;)Ljava/lang/String;");
    jmethodID asBoolean = env->GetStaticMethodID(javaBridgeUtilClass, "asBoolean",
                                                 "(Ljava/lang/Object;)Z");
    jmethodID getArrayElement = env->GetStaticMethodID(javaBridgeUtilClass, "getArrayElement",
                                                       "(Ljava/lang/Object;I)Ljava/lang/Object;");
    jmethodID isArray = env->GetMethodID(javaClass, "isArray", "()Z");
    jmethodID getComponentType = env->GetMethodID(javaClass, "getComponentType",
                                                  "()Ljava/lang/Class;");
    jmethodID getName = env->GetMethodID(javaClass, "getName", "()Ljava/lang/String;");

    v8::Local<v8::Value> javaClassValue;
    v8::Local<v8::Value> value;

    if (javaObject == nullptr) {
        javaClassValue = v8::Null(isolate);
        value = v8::Undefined(isolate);

        result->Set(context, v8::String::NewFromUtf8Literal(isolate, "javaClass"),
                    javaClassValue).Check();
        result->Set(context, v8::String::NewFromUtf8Literal(isolate, "value"), value).Check();
        return result;
    }

    if (env->CallBooleanMethod(typeClass, isArray)) {
        javaClassValue = v8::String::NewFromUtf8Literal(isolate, "array");
        jobject componentTypeClass = env->CallObjectMethod(typeClass, getComponentType);

        jobjectArray javaArray = static_cast<jobjectArray>(javaObject);
        int length = env->GetArrayLength(javaArray);
        v8::Local<v8::Array> array = v8::Array::New(isolate, length);

        for (int index = 0; index < length; index++) {
            jobject theObject = env->CallStaticObjectMethod(javaBridgeUtilClass, getArrayElement,
                                                            javaArray, index);
            array->Set(context, index,
                       wrapAsObject(isolate, context, theObject, componentTypeClass)).Check();
        }

        value = array;
    } else {
        const char *type = Util::JavaStr2CStr(
                static_cast<jstring>(env->CallObjectMethod(typeClass, getName)));

        if (
                !strcmp(type, "byte") ||
                !strcmp(type, "short") ||
                !strcmp(type, "int") ||
                !strcmp(type, "long") ||
                !strcmp(type, "float") ||
                !strcmp(type, "double")
                ) {
            javaClassValue = v8::Null(isolate);
            value = v8::Number::New(isolate,
                                    env->CallStaticDoubleMethod(javaBridgeUtilClass, asNumber,
                                                                javaObject));
        } else if (!strcmp(type, "boolean")) {
            javaClassValue = v8::Null(isolate);
            value = v8::Boolean::New(isolate,
                                     env->CallStaticBooleanMethod(javaBridgeUtilClass, asBoolean,
                                                                  javaObject));
        } else if (!strcmp(type, "char") || !strcmp(type, "java.lang.String")) {
            javaClassValue = v8::Null(isolate);
            value = v8::String::NewFromUtf8(isolate, Util::JavaStr2CStr(
                    static_cast<jstring>(env->CallStaticObjectMethod(javaBridgeUtilClass, asString,
                                                                     javaObject)))).ToLocalChecked();
        } else {

            javaClassValue = v8::String::NewFromUtf8(isolate, type).ToLocalChecked();
            value = v8::Null(isolate);
        }
    }

    result->Set(context, v8::String::NewFromUtf8Literal(isolate, "javaClass"),
                javaClassValue).Check();
    result->Set(context, v8::String::NewFromUtf8Literal(isolate, "value"), value).Check();
    return result;
}

v8::Local<v8::Object> Accessor::BuildFieldObject(
        v8::Isolate *isolate,
        v8::Local<v8::Context> context,
        v8::Local<v8::Value> self,
        v8::Local<v8::String> fieldName
) {
    JNIEnv *env = Main::env();
    jclass javaBridgeUtilClass = env->FindClass("com/mucheng/nodejava/javabridge/JavaBridgeUtil");
    jclass fieldClass = env->FindClass("java/lang/reflect/Field");
    jmethodID setFieldAccessible = env->GetMethodID(fieldClass, "setAccessible", "(Z)V");
    jmethodID getFieldByClassName = env->GetStaticMethodID(javaBridgeUtilClass, "getField",
                                                           "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/reflect/Field;");
    jmethodID getFieldByInstance = env->GetStaticMethodID(javaBridgeUtilClass, "getField",
                                                          "(Ljava/lang/Object;Ljava/lang/String;)Ljava/lang/reflect/Field;");
    jmethodID getFieldType = env->GetMethodID(fieldClass, "getType", "()Ljava/lang/Class;");
    jmethodID get = env->GetMethodID(fieldClass, "get",
                                     "(Ljava/lang/Object;)Ljava/lang/Object;");

    if (self->IsFunction()) {
        v8::Local<v8::Function> constructor = self.As<v8::Function>();
        v8::Local<v8::String> className = constructor->Get(context, v8::Symbol::For(isolate,
                                                                                    v8::String::NewFromUtf8Literal(
                                                                                            isolate,
                                                                                            "className"))).ToLocalChecked().As<v8::String>();
        jobject fieldObject = env->CallStaticObjectMethod(javaBridgeUtilClass, getFieldByClassName,
                                                          Util::CStr2JavaStr(
                                                                  *v8::String::Utf8Value(isolate,
                                                                                         className)),
                                                          Util::CStr2JavaStr(
                                                                  *v8::String::Utf8Value(isolate,
                                                                                         fieldName)));

        if (env->ExceptionCheck()) {
            jthrowable throwable = env->ExceptionOccurred();
            env->ExceptionClear();
            Util::ThrowExceptionToJS(isolate, throwable);
            return v8::Object::New(isolate);
        }

        env->CallVoidMethod(fieldObject, setFieldAccessible, true);
        jobject javaObject = env->CallObjectMethod(fieldObject, get, nullptr);

        if (env->ExceptionCheck()) {
            jthrowable throwable = env->ExceptionOccurred();
            env->ExceptionClear();
            Util::ThrowExceptionToJS(isolate, throwable);
            return v8::Object::New(isolate);
        }

        return wrapAsObject(isolate, context, javaObject,
                            env->CallObjectMethod(fieldObject, getFieldType));
    }

    v8::Local<v8::Object> instance = self.As<v8::Object>();
    v8::Local<v8::External> ref = instance->Get(context, v8::Symbol::For(isolate,
                                                                         v8::String::NewFromUtf8Literal(
                                                                                 isolate,
                                                                                 "__ref__"))).ToLocalChecked().As<v8::External>();


    jobject fieldObject = env->CallStaticObjectMethod(javaBridgeUtilClass, getFieldByInstance,
                                                      ref->Value(),
                                                      Util::CStr2JavaStr(
                                                              *v8::String::Utf8Value(isolate,
                                                                                     fieldName)));

    if (env->ExceptionCheck()) {
        jthrowable throwable = env->ExceptionOccurred();
        env->ExceptionClear();
        Util::ThrowExceptionToJS(isolate, throwable);
        return v8::Object::New(isolate);
    }

    env->CallVoidMethod(fieldObject, setFieldAccessible, true);
    jobject javaObject = env->CallObjectMethod(fieldObject, get, ref->Value());

    if (env->ExceptionCheck()) {
        jthrowable throwable = env->ExceptionOccurred();
        env->ExceptionClear();
        Util::ThrowExceptionToJS(isolate, throwable);
        return v8::Object::New(isolate);
    }

    return wrapAsObject(isolate, context, javaObject,
                        env->CallObjectMethod(fieldObject, getFieldType));
}

v8::Local<v8::Object> Accessor::BuildMethodObject(
        v8::Isolate *isolate,
        v8::Local<v8::Context> context,
        v8::Local<v8::Value> self,
        v8::Local<v8::String> methodName,
        v8::Local<v8::Array> args
) {
    JNIEnv *env = Main::env();
    jclass javaBridgeUtilClass = env->FindClass("com/mucheng/nodejava/javabridge/JavaBridgeUtil");
    jclass methodClass = env->FindClass("java/lang/reflect/Method");
    jclass objectClass = env->FindClass("java/lang/Object");
    jclass javaDoubleClass = env->FindClass("java/lang/Double");
    jclass javaBooleanClass = env->FindClass("java/lang/Boolean");
    jmethodID setMethodAccessible = env->GetMethodID(methodClass, "setAccessible", "(Z)V");
    jmethodID getMethodByClassName = env->GetStaticMethodID(javaBridgeUtilClass, "getMethod",
                                                            "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/Object;)[Ljava/lang/Object;");
    jmethodID getMethodByInstance = env->GetStaticMethodID(javaBridgeUtilClass, "getMethod",
                                                           "(Ljava/lang/Object;Ljava/lang/String;[Ljava/lang/Object;)[Ljava/lang/Object;");
    jmethodID getMethodType = env->GetMethodID(methodClass, "getReturnType", "()Ljava/lang/Class;");
    jmethodID invoke = env->GetMethodID(methodClass, "invoke",
                                        "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");
    jmethodID doubleValueOf = env->GetStaticMethodID(javaDoubleClass, "valueOf",
                                                     "(D)Ljava/lang/Double;");
    jmethodID booleanValueOf = env->GetStaticMethodID(javaBooleanClass, "valueOf",
                                                      "(Z)Ljava/lang/Boolean;");

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
                return v8::Object::New(isolate);
            }
        }
    }

    if (self->IsFunction()) {
        v8::Local<v8::Function> constructor = self.As<v8::Function>();
        v8::Local<v8::String> className = constructor->Get(context, v8::Symbol::For(isolate,
                                                                                    v8::String::NewFromUtf8Literal(
                                                                                            isolate,
                                                                                            "className"))).ToLocalChecked().As<v8::String>();
        jobjectArray resultArray = static_cast<jobjectArray>(env->CallStaticObjectMethod(
                javaBridgeUtilClass, getMethodByClassName,
                Util::CStr2JavaStr(*v8::String::Utf8Value(isolate, className)),
                Util::CStr2JavaStr(
                        *v8::String::Utf8Value(isolate,
                                               methodName)),
                params));

        if (env->ExceptionCheck()) {
            jthrowable throwable = env->ExceptionOccurred();
            env->ExceptionClear();
            Util::ThrowExceptionToJS(isolate, throwable);
            return v8::Object::New(isolate);
        }

        jobject method = env->GetObjectArrayElement(resultArray, 0);
        jobjectArray finalParams = static_cast<jobjectArray>(env->GetObjectArrayElement(resultArray,
                                                                                        1));
        env->CallVoidMethod(method, setMethodAccessible, true);
        jobject javaObject = env->CallObjectMethod(method, invoke, nullptr, finalParams);


        if (env->ExceptionCheck()) {
            jthrowable throwable = env->ExceptionOccurred();
            env->ExceptionClear();
            Util::ThrowExceptionToJS(isolate, throwable);
            return v8::Object::New(isolate);
        }

        return wrapAsObject(isolate, context, javaObject,
                            env->CallObjectMethod(method, getMethodType));
    }

    v8::Local<v8::Object> instance = self.As<v8::Object>();
    v8::Local<v8::External> ref = instance->Get(context, v8::Symbol::For(isolate,
                                                                         v8::String::NewFromUtf8Literal(
                                                                                 isolate,
                                                                                 "__ref__"))).ToLocalChecked().As<v8::External>();

    jobjectArray resultArray = static_cast<jobjectArray>(env->CallStaticObjectMethod(
            javaBridgeUtilClass, getMethodByInstance,
            ref->Value(),
            Util::CStr2JavaStr(
                    *v8::String::Utf8Value(isolate,
                                           methodName)),
            params));

    if (env->ExceptionCheck()) {
        jthrowable throwable = env->ExceptionOccurred();
        env->ExceptionClear();
        Util::ThrowExceptionToJS(isolate, throwable);
        return v8::Object::New(isolate);
    }

    jobject method = env->GetObjectArrayElement(resultArray, 0);
    jobjectArray finalParams = static_cast<jobjectArray>(env->GetObjectArrayElement(resultArray,
                                                                                    1));
    env->CallVoidMethod(method, setMethodAccessible, true);
    jobject javaObject = env->CallObjectMethod(method, invoke, ref->Value(), finalParams);

    if (env->ExceptionCheck()) {
        jthrowable throwable = env->ExceptionOccurred();
        env->ExceptionClear();
        Util::ThrowExceptionToJS(isolate, throwable);
        return v8::Object::New(isolate);
    }

    return wrapAsObject(isolate, context, javaObject,
                        env->CallObjectMethod(method, getMethodType));
}
