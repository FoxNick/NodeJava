//
// Created by 35785 on 2024/1/27.
//

#ifndef NODEJAVA_WRAPPER_H
#define NODEJAVA_WRAPPER_H

#include <jni.h>
#include <v8.h>

class Wrapper {
private:
    Wrapper() = delete;

public:
    static void WrapTo(
            v8::Isolate *isolate,
            v8::Local<v8::Context> context,
            v8::Local<v8::Object> to,
            jobject javaObject
    );

    static jobject Unwrap(
            v8::Isolate *isolate,
            v8::Local<v8::Context> context,
            v8::Local<v8::Object> from
    );

    static bool IsWrapped(
            v8::Isolate *isolate,
            v8::Local<v8::Context> context,
            v8::Local<v8::Object> target
    );

};


#endif //NODEJAVA_WRAPPER_H
