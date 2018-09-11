// Copyright (c) 2017 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.

#include "injection/common/wrapper/injection_wrapper.h"

namespace extensions_v8 {

InjectionWrapper::InjectionWrapper(const char* injection_name,
                                   const char* injection_source)
  : v8::Extension(injection_name, injection_source),
    cs_(new InjectionWrapper::CallbacksStorage) {}

InjectionWrapper::~InjectionWrapper() {}

v8::Handle<v8::FunctionTemplate> InjectionWrapper::GetNativeFunctionTemplate(
    InjectionWrapper::Isolate* isolate,
    v8::Handle<v8::String> function_name) {
  if (!cs_) {
      return v8::Handle<v8::FunctionTemplate>();
  }

  std::string name(*v8::String::Utf8Value(isolate, function_name->ToString()));

  CallbacksType::const_iterator c_it = cs_->callbacks_.find(name);

  if (c_it != cs_->callbacks_.end()) {
      return v8::FunctionTemplate::New(isolate, cs_->callbacks_[name]);
  }

  return v8::Handle<v8::FunctionTemplate>();
}

void InjectionWrapper::AddCallback(const std::string& callback_name,
                                   InjectionWrapper::Callback callback) {
  if (cs_) {
      cs_->callbacks_.insert(
          std::pair<std::string, InjectionWrapper::Callback>(
              callback_name, callback));
  }
}

// static
InjectionWrapper::LocalContext
InjectionWrapper::GetCurrentContext(InjectionWrapper::Isolate* isolate) {
  return isolate
      ? isolate->GetCurrentContext() : InjectionWrapper::LocalContext();
}

// static
InjectionWrapper::LocalFunction InjectionWrapper::LocalCastObjectToFunction(
    InjectionWrapper::LocalObject object) {
  return v8::Local<v8::Function>::Cast(object);
}

// static
InjectionWrapper::LocalBoolean InjectionWrapper::NewLocalBoolean(
    InjectionWrapper::Isolate* isolate,
    bool value) {
  return v8::Boolean::New(isolate, value);
}

// static
InjectionWrapper::LocalContext InjectionWrapper::NewLocalContextFromPersistent(
    InjectionWrapper::Isolate* isolate,
    const InjectionWrapper::PersistentContext& context) {
  return v8::Local<v8::Context>::New(isolate, context);
}

// static
InjectionWrapper::LocalInteger InjectionWrapper::NewLocalInteger(
    InjectionWrapper::Isolate* isolate,
    int32_t value) {
  return v8::Int32::New(isolate, value);
}

// static
InjectionWrapper::LocalObject InjectionWrapper::NewLocalObjectFromPersistent(
    InjectionWrapper::Isolate* isolate,
    const InjectionWrapper::PersistentObject& object) {
  return v8::Local<v8::Object>::New(isolate, object);
}

// static
InjectionWrapper::LocalString InjectionWrapper::NewLocalStringFromUtf8(
    InjectionWrapper::Isolate* isolate,
    const char* data,
    InjectionWrapper::NewStringType type,
    int length) {
  return v8::String::NewFromUtf8(isolate, data, type, length);
}

InjectionWrapper::CallbacksStorage::CallbacksStorage() {}

InjectionWrapper::CallbacksStorage::~CallbacksStorage() {}

// JSFunctionCallbackInfoValue
JSFunctionCallbackInfoValue::JSFunctionCallbackInfoValue(
    const InjectionWrapper::CallbackInfoValue& args)
  : args_(args) {}

JSFunctionCallbackInfoValue::~JSFunctionCallbackInfoValue() {}

InjectionWrapper::Isolate* JSFunctionCallbackInfoValue::GetIsolate() const {
  return args_.GetIsolate();
}

InjectionWrapper::ReturnValue
JSFunctionCallbackInfoValue::GetReturnValue() const {
  return args_.GetReturnValue();
}

int JSFunctionCallbackInfoValue::Length() const {
  return args_.Length();
}

InjectionWrapper::LocalValue
JSFunctionCallbackInfoValue::operator[](int i) const {
  return args_[i];
}

// JSMaybeInt32
JSMaybeInt32::JSMaybeInt32(const InjectionWrapper::MaybeInt32& maybe_int32)
  : maybe_int32_(maybe_int32) {}

JSMaybeInt32::~JSMaybeInt32() {}

int32_t JSMaybeInt32::FromJust() const {
  return maybe_int32_.FromJust();
}

// JSMaybeLocalString
JSMaybeLocalString::JSMaybeLocalString(
    const InjectionWrapper::MaybeLocalString& maybe_local_string)
  : maybe_local_string_(maybe_local_string) {}

JSMaybeLocalString::~JSMaybeLocalString() {}

InjectionWrapper::LocalString JSMaybeLocalString::FromMaybe(
    InjectionWrapper::LocalString default_string) const {
  return maybe_local_string_.FromMaybe(default_string);
}

// JSReturnValue
JSReturnValue::JSReturnValue(const InjectionWrapper::ReturnValue& return_value)
  : return_value_(return_value) {}

JSReturnValue::~JSReturnValue() {}

void JSReturnValue::Set(const InjectionWrapper::LocalString handle) {
  return_value_.Set(handle);
}

// JSValue
JSValue::JSValue(const InjectionWrapper::LocalValue& value)
  : value_(value) {}

JSValue::~JSValue() {}

InjectionWrapper::MaybeInt32
JSValue::Int32Value(InjectionWrapper::LocalContext context) const {
  return value_->Int32Value(context);
}

bool JSValue::IsArray() const {
  return value_->IsArray();
}

bool JSValue::IsFunction() const {
  return value_->IsFunction();
}

bool JSValue::IsInt32() const {
  return value_->IsInt32();
}

bool JSValue::IsNull() const {
  return value_->IsNull();
}

bool JSValue::IsObject() const {
  return value_->IsObject();
}

bool JSValue::IsString() const {
  return value_->IsString();
}

bool JSValue::IsStringObject() const {
  return value_->IsStringObject();
}

InjectionWrapper::MaybeLocalObject JSValue::ToObject(
      InjectionWrapper::LocalContext context) const {
  return value_->ToObject(context);
}

InjectionWrapper::MaybeLocalString JSValue::ToString(
    InjectionWrapper::LocalContext context) const {
  return value_->ToString(context);
}

// JSUtf8Value
JSUtf8Value::JSUtf8Value(InjectionWrapper::Isolate* isolate,
                         const InjectionWrapper::LocalValue& value)
  : utf8_string_(isolate, value) {}

JSUtf8Value::~JSUtf8Value() {}

char* JSUtf8Value::operator*() {
  return *utf8_string_;
}

const char* JSUtf8Value::operator*() const {
  return *utf8_string_;
}

int JSUtf8Value::Length() const {
  return utf8_string_.length();
}

}  // namespace extensions_v8
