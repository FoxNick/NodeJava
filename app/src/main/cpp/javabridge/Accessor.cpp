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
    jclass methodClass = env->FindClass("java/lang/reflect/Method");
    jmethodID setFieldAccessible = env->GetMethodID(fieldClass, "setAccessible", "(Z)V");
    jmethodID setMethodAccessible = env->GetMethodID(methodClass, "setAccessible", "(Z)V");
    jmethodID getField = env->GetStaticMethodID(javaBridgeUtilClass, "getField",
                                                "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/reflect/Field;");
    jmethodID getMethod = env->GetStaticMethodID(javaBridgeUtilClass, "getMethod",
                                                 "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/reflect/Method;");
    jmethodID getFieldType = env->GetMethodID(fieldClass, "getType", "()Ljava/lang/Class;");
    jmethodID getMethodType = env->GetMethodID(methodClass, "getReturnType", "()Ljava/lang/Class;");
    jmethodID get = env->GetMethodID(fieldClass, "get",
                                     "(Ljava/lang/Object;)Ljava/lang/Object;");
    jmethodID invoke = env->GetMethodID(methodClass, "invoke",
                                        "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");

    if (self->IsFunction()) {
        v8::Local<v8::Function> constructor = self.As<v8::Function>();
        v8::Local<v8::String> className = constructor->Get(context, v8::Symbol::For(isolate,
                                                                                    v8::String::NewFromUtf8Literal(
                                                                                            isolate,
                                                                                            "className"))).ToLocalChecked().As<v8::String>();
        jobject fieldObject = env->CallStaticObjectMethod(javaBridgeUtilClass, getField,
                                                          Util::CStr2JavaStr(
                                                                  *v8::String::Utf8Value(isolate,
                                                                                         className)),
                                                          Util::CStr2JavaStr(
                                                                  *v8::String::Utf8Value(isolate,
                                                                                         fieldName)));
        env->CallVoidMethod(fieldObject, setFieldAccessible, true);
        jobject javaObject = env->CallObjectMethod(fieldObject, get, nullptr);
        return wrapAsObject(isolate, context, javaObject,
                            env->CallObjectMethod(fieldObject, getFieldType));
    }
    return v8::Local<v8::Object>();
}

v8::Local<v8::Object> Accessor::BuildMethodObject(
        v8::Isolate *isolate,
        v8::Local<v8::Context> context,
        v8::Local<v8::Value> self,
        v8::Local<v8::String> methodName
) {
    JNIEnv *env = Main::env();
    jclass javaBridgeUtilClass = env->FindClass("com/mucheng/nodejava/javabridge/JavaBridgeUtil");
    jclass fieldClass = env->FindClass("java/lang/reflect/Field");
    jclass methodClass = env->FindClass("java/lang/reflect/Method");
    jmethodID setFieldAccessible = env->GetMethodID(fieldClass, "setAccessible", "(Z)V");
    jmethodID setMethodAccessible = env->GetMethodID(methodClass, "setAccessible", "(Z)V");
    jmethodID getField = env->GetStaticMethodID(javaBridgeUtilClass, "getField",
                                                "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/reflect/Field;");
    jmethodID getMethod = env->GetStaticMethodID(javaBridgeUtilClass, "getMethod",
                                                 "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/reflect/Method;");
    jmethodID getFieldType = env->GetMethodID(fieldClass, "getType", "()Ljava/lang/Class;");
    jmethodID getMethodType = env->GetMethodID(methodClass, "getReturnType", "()Ljava/lang/Class;");
    jmethodID get = env->GetMethodID(fieldClass, "get",
                                     "(Ljava/lang/Object;)Ljava/lang/Object;");
    jmethodID invoke = env->GetMethodID(methodClass, "invoke",
                                        "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");

    if (self->IsFunction()) {
        v8::Local<v8::Function> constructor = self.As<v8::Function>();
        v8::Local<v8::String> className = constructor->Get(context, v8::Symbol::For(isolate,
                                                                                    v8::String::NewFromUtf8Literal(
                                                                                            isolate,
                                                                                            "className"))).ToLocalChecked().As<v8::String>();
        jobject methodObject = env->CallStaticObjectMethod(javaBridgeUtilClass, getMethod,
                                                           Util::CStr2JavaStr(
                                                                   *v8::String::Utf8Value(isolate,
                                                                                          className)),
                                                           Util::CStr2JavaStr(
                                                                   *v8::String::Utf8Value(isolate,
                                                                                          methodName)));
        env->CallVoidMethod(methodObject, setMethodAccessible, true);
        jobject javaObject = env->CallObjectMethod(methodObject, invoke, nullptr,
                                                   env->NewObjectArray(0, env->FindClass(
                                                                               "java/lang/Object"),
                                                                       nullptr));
        return wrapAsObject(isolate, context, javaObject,
                            env->CallObjectMethod(methodObject, getMethodType));
    }
    return v8::Local<v8::Object>();
}
