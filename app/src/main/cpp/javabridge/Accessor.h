//
// Created by 35785 on 2024/1/26.
//

#ifndef NODEJAVA_ACCESSOR_H
#define NODEJAVA_ACCESSOR_H

#include <v8.h>
#include <jni.h>

class Accessor {
private:
    Accessor() = delete;

public:
    static v8::Local<v8::Object> wrapAsObject(
            v8::Isolate *isolate,
            v8::Local<v8::Context> context,
            jobject javaObject,
            jobject typeClass
    );

    static v8::Local<v8::Object> BuildFieldObject(
            v8::Isolate *isolate,
            v8::Local<v8::Context> context,
            v8::Local<v8::Value> self,
            v8::Local<v8::String> fieldName
    );

    static v8::Local<v8::Object> BuildMethodObject(
            v8::Isolate *isolate,
            v8::Local<v8::Context> context,
            v8::Local<v8::Value> self,
            v8::Local<v8::String> methodName,
            v8::Local<v8::Array> args
    );
};


#endif //NODEJAVA_ACCESSOR_H
