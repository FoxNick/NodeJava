#include "Wrapper.h"

void
Wrapper::WrapTo(
        v8::Isolate *isolate,
        v8::Local<v8::Context> context,
        v8::Local<v8::Object> to,
        jobject javaObject
) {
    to->Set(context, v8::Symbol::For(isolate, v8::String::NewFromUtf8Literal(isolate, "__ref__")),
            v8::External::New(isolate, javaObject)).Check();
}

jobject
Wrapper::Unwrap(
        v8::Isolate *isolate,
        v8::Local<v8::Context> context,
        v8::Local<v8::Object> from
) {
    return static_cast<jobject>(from->Get(context, v8::Symbol::For(isolate,
                                                                   v8::String::NewFromUtf8Literal(
                                                                           isolate,
                                                                           "__ref__"))).ToLocalChecked().As<v8::External>()->Value());
}

bool Wrapper::IsWrapped(
        v8::Isolate *isolate,
        v8::Local<v8::Context> context,
        v8::Local<v8::Object> target
) {

    v8::MaybeLocal<v8::Value> maybeLocal = target->Get(context, v8::Symbol::For(isolate,
                                                                                v8::String::NewFromUtf8Literal(
                                                                                        isolate,
                                                                                        "__ref__")));
    if (maybeLocal.IsEmpty()) {
        return false;
    }
    return maybeLocal.ToLocalChecked()->IsExternal();
}
