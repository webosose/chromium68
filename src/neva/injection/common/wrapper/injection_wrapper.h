// Copyright (c) 2017 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.


#ifndef INJECTION_COMMON_WRAPPER_INJECTION_WRAPPER_H_
#define INJECTION_COMMON_WRAPPER_INJECTION_WRAPPER_H_

#include <map>
#include <string>

#include "v8/include/v8.h"

namespace extensions_v8 {

// the macro is intended to make it a bit more handy and safe to employ the
// InjectionWrapper::addCallback function declared below
#define IW_ADDCALLBACK(callback_name) AddCallback(#callback_name, callback_name)

// This is a wrapper type for the v8::Extension type, which is intended to
// reduce as much as possible dependency on the v8 sources while dealing to
// LGE v8 extensions (injections).
// One has just to do the following (to make use of it):
//   (1) publicly derive from that type
//   (2) list (and implement) native functions (if any) with the following
//       signature:
//         static void Foo(const InjectionWrapper::callbackInfoValue& args);
//   (3) register these native functions within your c-tor as follows:
//         addCallback("Foo", Foo);
class InjectionWrapper : public v8::Extension {
 public:
  // the types to be used by injection developers
  typedef v8::Context::Scope ContextScope;
  typedef v8::FunctionCallback Callback;
  typedef v8::FunctionCallbackInfo<v8::Value> CallbackInfoValue;
  typedef v8::HandleScope HandleScope;
  typedef v8::Isolate Isolate;
  typedef v8::Isolate::Scope Scope;
  typedef v8::Local<v8::Boolean> LocalBoolean;
  typedef v8::Local<v8::Context> LocalContext;
  typedef v8::Local<v8::Function> LocalFunction;
  typedef v8::Local<v8::Integer> LocalInteger;
  typedef v8::Local<v8::Object> LocalObject;
  typedef v8::Local<v8::String> LocalString;
  typedef v8::Local<v8::Value> LocalValue;
  typedef v8::Maybe<int32_t> MaybeInt32;
  typedef v8::MaybeLocal<v8::Object> MaybeLocalObject;
  typedef v8::MaybeLocal<v8::String> MaybeLocalString;
  typedef v8::Persistent<v8::Context> PersistentContext;
  typedef v8::Persistent<v8::Object> PersistentObject;
  typedef v8::ReturnValue<v8::Value> ReturnValue;
  typedef v8::String::NewStringType NewStringType;
  typedef v8::String::Utf8Value Utf8String;

  // alias types to be used by injection developers
  template <typename T>
  using Local = v8::Local<T>;

  InjectionWrapper(const char* injection_name,
                   const char* injection_source = 0);
  ~InjectionWrapper() override;

  // InjectionWrapper interface

  // mapping registered callbacks (native functions)
  v8::Handle<v8::FunctionTemplate> GetNativeFunctionTemplate(
      Isolate* isolate,
      v8::Handle<v8::String> function_name) override;

  // wrapped v8::Local<v8::Context> v8::Isolate::GetCurrentContext()
  static LocalContext GetCurrentContext(Isolate* isolate);

  // wrapped static v8::Local<v8::Function> v8::Local::Cast(
  // v8::Local<v8::Object> that)
  static LocalFunction LocalCastObjectToFunction(LocalObject object);

  // wrapped static v8::Local<v8::Boolean> v8::Boolean::New(
  // v8::Isolate* isolate, bool value)
  static LocalBoolean NewLocalBoolean(Isolate* isolate, bool value);

  // wrapped static v8::Local<v8::Context> v8::Local::New(v8::Isolate* isolate,
  // const v8::PersistentBase<v8::Context>& that)
  static LocalContext NewLocalContextFromPersistent(
      Isolate* isolate,
      const PersistentContext& context);

  // wrapped static v8::Local<v8::Integer> v8::Integer::New(
  // v8::Isolate* isolate, int32_t value)
  static LocalInteger NewLocalInteger(Isolate* isolate, int32_t value);

  // wrapped static v8::Local<v8::Object> New(v8::Isolate* isolate,
  // const v8::PersistentBase<v8::Object>& that)
  static LocalObject NewLocalObjectFromPersistent(
      Isolate* isolate,
      const PersistentObject& object);

  // wrapped static v8::Local<v8::String> v8::String::NewFromUtf8(
  // v8::Isolate* isolate, const char* data, v8::String::NewStringType type,
  // int length)
  static LocalString NewLocalStringFromUtf8(
      Isolate* isolate,
      const char* data,
      NewStringType type = NewStringType::kNormalString,
      int length = -1);

 protected:
  // registering a callback (native function)
  void AddCallback(const std::string& callback_name, Callback callback);
  // and more (if reasonable)...

 private:
  typedef std::map<std::string, Callback> CallbacksType;

  // a dedicated (private inner) callbacks storage class (to store a callbacks
  // vocabulary for an injection separately)
  struct CallbacksStorage {
    explicit CallbacksStorage();
    ~CallbacksStorage();
    CallbacksType callbacks_;
  };

  // using a pointer to the callbacks storage (instead of maintaining it within
  // the wrapper) safes memory
  std::unique_ptr<CallbacksStorage> cs_;
};

// Since the v8::FunctionCallbackInfo<v8::Value> type is used regularly, it
// seems unreasonable to employ the pointer-to-implementation idiom while
// wrapping the type.
// One has just to explicitly contsruct wrapper instance for the
// v8::FunctionCallbackInfo<v8::Value> type and use required interface if
// supported (to be extended by request).
class JSFunctionCallbackInfoValue {
 public:
  explicit JSFunctionCallbackInfoValue(
      const InjectionWrapper::CallbackInfoValue& args);
  ~JSFunctionCallbackInfoValue();

  // JSFunctionCallbackInfoValue public interface

  // wrapped v8::Isolate* FunctionCallbackInfo<v8::Value>::GetIsolate() const
  InjectionWrapper::Isolate* GetIsolate() const;
  // wrapped v8::ReturnValue<v8::Value>
  // FunctionCallbackInfo<v8::Value>::GetReturnValue() const
  InjectionWrapper::ReturnValue GetReturnValue() const;
  // wrapped int FunctionCallbackInfo<v8::Value>::Length() const
  int Length() const;
  // wrapped v8::Local<v8::Value>
  // FunctionCallbackInfo<v8::Value>::operator[](int i) const
  InjectionWrapper::LocalValue operator[](int i) const;
  // and more (if reasonable)...

 private:
  // not implemented
  JSFunctionCallbackInfoValue(const JSFunctionCallbackInfoValue&);
  JSFunctionCallbackInfoValue& operator=(const JSFunctionCallbackInfoValue&);

  InjectionWrapper::CallbackInfoValue args_;
};

// Since the v8::Local<T> type is used regularly, it seems unreasonable to
// employ the pointer-to-implementation idiom while wrapping the type.
// One has just to explicitly contsruct wrapper instance for the v8::Local<T>
// type and use required interface if supported (to be extended by request).
template <typename T>
class JSLocal {
 public:
  explicit JSLocal(const InjectionWrapper::Local<T>& local) : local_(local) {}
  ~JSLocal() {}

  // JSLocal public interface

  // wrapped v8::Local<S> v8::Local<T>As() const
  template <typename S>
  InjectionWrapper::Local<S> As() const {
    return local_.As();
  }
  // and more (if reasonable)...

 private:
  // not implemented
  JSLocal(const JSLocal&);
  JSLocal& operator=(const JSLocal&);

  InjectionWrapper::Local<T> local_;
};

// Since the v8::Maybe<int32_t> type is used regularly, it seems unreasonable to
// employ the pointer-to-implementation idiom while wrapping the type.
// One has just to explicitly contsruct wrapper instance for the
// v8::Maybe<int32_t> type and use required interface if supported (to be
// extended by request).
class JSMaybeInt32 {
 public:
  explicit JSMaybeInt32(
      const InjectionWrapper::MaybeInt32& maybe_int32);
  ~JSMaybeInt32();

  // JSMaybeInt32 public interface

  // wrapped int32_t v8::Maybe<int32_t>::FromJust() const
  int32_t FromJust() const;
  // and more (if reasonable)...

 private:
  // not implemented
  JSMaybeInt32(const JSMaybeInt32&);
  JSMaybeInt32& operator=(const JSMaybeInt32&);

  InjectionWrapper::MaybeInt32 maybe_int32_;
};

// Since the v8::MaybeLocal<v8::String> type is used regularly, it seems
// unreasonable to employ the pointer-to-implementation idiom while wrapping the
// type. One has just to explicitly contsruct wrapper instance for the
// v8::MaybeLocal<v8::String> type and use required interface if supported (to
// be extended by request).
class JSMaybeLocalString {
 public:
  explicit JSMaybeLocalString(
      const InjectionWrapper::MaybeLocalString& maybe_local_string);
  ~JSMaybeLocalString();

  // JSMaybeLocalString public interface

  // wrapped v8::Local<v8::String>
  // v8::MaybeLocal<v8::String>::FromMaybe(v8::Local<v8::String> default_value)
  // const
  InjectionWrapper::LocalString FromMaybe(
      InjectionWrapper::LocalString default_string) const;
  // and more (if reasonable)...

 private:
  // not implemented
  JSMaybeLocalString(const JSMaybeLocalString&);
  JSMaybeLocalString& operator=(const JSMaybeLocalString&);

  InjectionWrapper::MaybeLocalString maybe_local_string_;
};

// Since the v8::ReturnValue type is used regularly, it seems unreasonable to
// employ the pointer-to-implementation idiom while wrapping the type.
// One has just to explicitly contsruct wrapper instance for the v8::ReturnValue
// type and use required interface if supported (to be extended by request).
class JSReturnValue {
 public:
  explicit JSReturnValue(const InjectionWrapper::ReturnValue& return_value);
  ~JSReturnValue();

  // JSReturnValue public interface

  // wrapped void v8::ReturnValue<v8::Value>::Set(const v8::Local<S> handle);
  void Set(const InjectionWrapper::LocalString handle);
  // and more (if reasonable)...

 private:
  // not implemented
  JSReturnValue(const JSReturnValue&);
  JSReturnValue& operator=(const JSReturnValue&);

  InjectionWrapper::ReturnValue return_value_;
};

// Since the v8::Value type is used regularly, it seems unreasonable to employ
// the pointer-to-implementation idiom while wrapping the type.
// One has just to explicitly contsruct wrapper instance for the v8::Value type
// and use required interface if supported (to be extended by request).
class JSValue {
 public:
  explicit JSValue(const InjectionWrapper::LocalValue& value);
  ~JSValue();

  // JSValue public interface

  // wrapped v8::Maybe<int32_t> v8::Value::Int32Value(
  // v8::Local<v8::Context> context) const
  InjectionWrapper::MaybeInt32 Int32Value(
      InjectionWrapper::LocalContext context) const;
  // wrapped bool v8::Value::IsArray() const
  bool IsArray() const;
  // wrapped bool v8::Value::IsFunction() const
  bool IsFunction() const;
  // wrapped bool v8::Value::IsInt32() const
  bool IsInt32() const;
  // wrapped bool v8::Value::IsNull() const
  bool IsNull() const;
  // wrapped bool v8::Value::IsObject() const
  bool IsObject() const;
  // wrapped bool v8::Value::IsString() const
  bool IsString() const;
  // wrapped bool v8::Value::IsStringObject() const
  bool IsStringObject() const;
  // wrapped v8::MaybeLocal<v8::Object> v8::Value::ToObject(
  // v8::Local<v8::Context> context) const
  InjectionWrapper::MaybeLocalObject ToObject(
      InjectionWrapper::LocalContext context) const;
  // wrapped v8::MaybeLocal<v8::String> v8::Value::ToString(
  // v8::Local<v8::Context> context) const
  InjectionWrapper::MaybeLocalString ToString(
      InjectionWrapper::LocalContext context) const;
  // and more (if reasonable)...

 private:
  // not implemented
  JSValue(const JSValue&);
  JSValue& operator=(const JSValue&);

  InjectionWrapper::LocalValue value_;
};

// Since the v8::JSUtf8Value type is used regularly, it seems unreasonable to
// employ the pointer-to-implementation idiom while wrapping the type.
// One has just to explicitly contsruct wrapper instance for the v8::JSUtf8Value
// type and use required interface if supported (to be extended by request).
class JSUtf8Value {
 public:
  explicit JSUtf8Value(InjectionWrapper::Isolate* isolate,
                       const InjectionWrapper::LocalValue& value);
  ~JSUtf8Value();

  // JSUtf8Value public interface

  char* operator*();
  const char* operator*() const;
  int Length() const;

 private:
  // not implemented
  JSUtf8Value(const JSUtf8Value&);
  JSUtf8Value& operator=(const JSUtf8Value&);

  InjectionWrapper::Utf8String utf8_string_;
};

}  // namespace extensions_v8

#endif  // INJECTION_COMMON_WRAPPER_INJECTION_WRAPPER_H_
