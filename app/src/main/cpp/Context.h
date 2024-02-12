#ifndef NODEJAVA_CONTEXT_H
#define NODEJAVA_CONTEXT_H

#include "Isolate.h"
#include <jni.h>

#define SETUP_CONTEXT_CLASS() v8::Local<v8::Context> v8Context = context->context_.Get(isolate->self);\
v8::Context::Scope contextScope(v8Context);

class Context {
public:
    Isolate *isolate;
    node::Environment *environment;
    v8::Persistent<v8::Context> context_;

    Context(Isolate *pEnvironment);

    static void To(jobject instance, Context *self);

    static Context *From(jobject instance);
};


#endif //NODEJAVA_CONTEXT_H
