// Copyright (c) 2017 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.

#include "injection/common/util/arguments_checking.h"

#include <base/logging.h>

#undef  BOOL_ALPHA
#define BOOL_ALPHA(bool_value) (bool_value ? "true" : "false")

namespace extensions_v8 {

// getCheckResultName
const char* GetCheckResultName(CheckResult result) {
  switch (result) {
    case CheckResult::RESULT_VALID:
      return "valid result";
    case CheckResult::RESULT_INVALID:
      return "invalid result";
    case CheckResult::RESULT_INVALID_LENGTH:
      return "invalid number of arguments";
    case CheckResult::RESULT_NOT_A_FUNCTION:
      return "argument is not a function";
    case CheckResult::RESULT_NOT_A_STRING:
      return "argument is not a string";
    case CheckResult::RESULT_NOT_AN_ARRAY:
      return "argument is not an array";
    case CheckResult::RESULT_NOT_AN_INT32:
      return "argument is not an int32";
    case CheckResult::RESULT_NOT_AN_OBJECT:
      return "argument is not an object";
    case CheckResult::RESULT_IS_NULL:
      return "argument is null";
    default:
      return "unknown result";
  }
}

// CheckArguments
CheckArguments::CheckArguments(const MethodArgsType& args)
    : args_(args),
      init_([](const MethodArgsType&) { return CheckResult::RESULT_VALID; }) {}

CheckArguments::CheckArguments(const CheckArguments& args) = default;

CheckArguments::~CheckArguments() {}

CheckArguments& CheckArguments::ArgumentsCount(int count) {
  return AddAndClause([count](const MethodArgsType& args) {
    return (JSFunctionCallbackInfoValue(args).Length() == count ?
        CheckResult::RESULT_VALID : CheckResult::RESULT_INVALID_LENGTH);
  });
}

MethodArgBinding CheckArguments::Argument(int index) {
  LOG(INFO) << __func__ << "(): called";
  return MethodArgBinding(index, *this);
}

CheckArguments::operator CheckResult() const {
  LOG(INFO) << __func__ << "(): called";
  return init_(args_);
}

CheckArguments& CheckArguments::AddAndClause(InitFunction right_condition) {
  InitFunction left_condition = init_, new_init =
      [left_condition, right_condition](const MethodArgsType& args) {
    CheckResult left_result = left_condition(args);
    return (left_result == CheckResult::RESULT_VALID ? right_condition(args) :
        left_result);
  };
  init_ = new_init;
  return *this;
}

// MethodArgBinding
MethodArgBinding::MethodArgBinding(int index, CheckArguments& parent)
  : arg_index_(index), parent_(parent) {}

MethodArgBinding& MethodArgBinding::IsArray() {
  auto bind = [this](const MethodArgsType& args) {
    bool is_array = JSValue(
        JSFunctionCallbackInfoValue(args)[arg_index_]).IsArray();
    LOG(INFO) << __func__ << "(): Checking whether argument is array: "
        << BOOL_ALPHA(is_array);
    return (is_array ? CheckResult::RESULT_VALID :
        CheckResult::RESULT_NOT_AN_ARRAY);
  };
  parent_.AddAndClause(bind);
  return *this;
}

MethodArgBinding& MethodArgBinding::IsFunction() {
  auto bind = [this](const MethodArgsType& args) {
    bool is_function = JSValue(
        JSFunctionCallbackInfoValue(args)[arg_index_]).IsFunction();
    LOG(INFO) << __func__ << "(): Checking whether argument is function: "
        << BOOL_ALPHA(is_function);
    return (is_function ? CheckResult::RESULT_VALID :
        CheckResult::RESULT_NOT_A_FUNCTION);
  };
  parent_.AddAndClause(bind);
  return *this;
}

MethodArgBinding& MethodArgBinding::IsInt32() {
  LOG(INFO) << __func__ << "(): called";
  auto bind = [this](const MethodArgsType& args) {
    bool is_int32 = JSValue(
        JSFunctionCallbackInfoValue(args)[arg_index_]).IsInt32();
    LOG(INFO) << __func__ << "(): Checking whether argument is int32: "
        << BOOL_ALPHA(is_int32);
    return (is_int32 ? CheckResult::RESULT_VALID :
        CheckResult::RESULT_NOT_AN_INT32);
  };
  parent_.AddAndClause(bind);
  return *this;
}

MethodArgBinding& MethodArgBinding::IsObject() {
  auto bind = [this](const MethodArgsType& args) {
    bool is_object = JSValue(
        JSFunctionCallbackInfoValue(args)[arg_index_]).IsObject();
    LOG(INFO) << __func__ << "(): Checking whether argument is object: "
        << BOOL_ALPHA(is_object);
    return (is_object ? CheckResult::RESULT_VALID :
        CheckResult::RESULT_NOT_AN_OBJECT);
  };
  parent_.AddAndClause(bind);
  return *this;
}

MethodArgBinding& MethodArgBinding::IsString() {
  LOG(INFO) << __func__ << "(): called";
  auto bind = [this](const MethodArgsType& args) {
    JSFunctionCallbackInfoValue js_args(args);
    JSValue js_arg(js_args[arg_index_]);
    bool is_string = js_arg.IsString() || js_arg.IsStringObject();
    LOG(INFO) << __func__ << "(): Checking whether argument is string: "
        << BOOL_ALPHA(is_string);
    return (is_string ? CheckResult::RESULT_VALID :
        CheckResult::RESULT_NOT_A_STRING);
  };
  parent_.AddAndClause(bind);
  return *this;
}

MethodArgBinding& MethodArgBinding::NotNull() {
  auto bind = [this](const MethodArgsType& args) {
    bool is_null = JSValue(
        JSFunctionCallbackInfoValue(args)[arg_index_]).IsNull();
    LOG(INFO) << __func__ << "(): Checking whether argument is not null: "
        << BOOL_ALPHA(is_null);
    return (is_null ? CheckResult::RESULT_IS_NULL : CheckResult::RESULT_VALID);
  };
  parent_.AddAndClause(bind);
  return *this;
}

CheckArguments& MethodArgBinding::BindInt32(int32_t& value) {
  return parent_.AddAndClause([this, &value](const MethodArgsType& args) {
    JSFunctionCallbackInfoValue js_args(args);
    JSValue js_arg(js_args[arg_index_]);
    InjectionWrapper::LocalContext js_context =
        InjectionWrapper::GetCurrentContext(js_args.GetIsolate());

    JSMaybeInt32 js_arg_int32(js_arg.Int32Value(js_context));
    value = js_arg_int32.FromJust();
    return CheckResult::RESULT_VALID;
  });
}

CheckArguments& MethodArgBinding::BindString(std::string& value) {
  return parent_.AddAndClause([this, &value](const MethodArgsType& args) {
    JSFunctionCallbackInfoValue js_args(args);
    JSValue js_arg(js_args[arg_index_]);
    InjectionWrapper::LocalContext js_context =
        InjectionWrapper::GetCurrentContext(js_args.GetIsolate());

    JSMaybeLocalString maybe_local_string(js_arg.ToString(js_context));
    JSUtf8Value val(js_args.GetIsolate(),
        maybe_local_string.FromMaybe(InjectionWrapper::LocalString()));

    value = *val;
    return CheckResult::RESULT_VALID;
  });
}

}  // namespace extensions_v8
