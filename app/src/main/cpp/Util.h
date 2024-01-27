//
// Created by 35785 on 2024/1/24.
//

#ifndef NODEJAVA_UTIL_H
#define NODEJAVA_UTIL_H
#include <v8.h>

namespace Util {
    void SetPtr(jclass javaClass, const char *fieldName, void *ptr);

    void SetPtr(jobject instance, const char *fieldName, void *ptr);

    void *GetPtr(jclass javaClass, const char *fieldName);

    void *GetPtr(jobject instance, const char *fieldName);

    v8::Local<v8::External> NewExternal(v8::Isolate *isolate, void *value);

    void ThrowExceptionToJS(v8::Isolate *isolate, jthrowable throwable);

    void ThrowScriptCompilingException(const char *message);

    void ThrowScriptRuntimeException(const char *message);

    void ThrowNodeException(const char *message);

    template<typename Class>
    inline Class As(long ptr) {
        return reinterpret_cast<Class>(ptr);
    }

    template<typename Class>
    inline Class GetPtrAs(jclass javaClass, const char *fieldName) {
        return static_cast<Class>(GetPtr(javaClass, fieldName));
    }

    template<typename Class>
    inline Class GetPtrAs(jobject instance, const char *fieldName) {
        return static_cast<Class>(GetPtr(instance, fieldName));
    }

    const char *JavaStr2CStr(jstring javaStr);

    jstring CStr2JavaStr(const char *cStr);
}

#endif //NODEJAVA_UTIL_H
