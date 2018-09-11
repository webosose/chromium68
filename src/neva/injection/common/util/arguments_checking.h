// Copyright (c) 2017 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.


#ifndef INJECTION_COMMON_UTIL_ARGUMENTS_CHECKING_H_
#define INJECTION_COMMON_UTIL_ARGUMENTS_CHECKING_H_

#include <cstdint>
#include <functional>

#include "injection/common/wrapper/injection_wrapper.h"

namespace extensions_v8 {

// Dealing to the v8-arguments while either implementing or maintaining native
// functions for an injection (LGE v8 extenstion) is not that complicated, but
// in most of the cases is too wordy (especially if there are many of arguments
// to be processed by the native function). The approach is pretty simple and
// obvious: one has to access an argument by a specific index, verify whether
// the argument is defined, check its type and just cast it to the proper C/C++
// type (by using any legal technique available in v8).
//
// Considering interaction with the v8 library, it seems quite reasonable to use
// an already implemented injection wrapper while dealing to v8-calls rather
// than use the v8 API directly (in that case the source code intended to
// reduce efforts of an injection developer while managing v8-arguments
// provided below won't be affected by any changes introduced in v8).
//
// The general idea of simplifying the v8-arguments processing is got from the
// global network resources. Anyway, the source code represented below is to be
// considered as a severe and thorough transformation of that idea meeting LGE
// Web API framework needs.

// while introducing new checking functions corresponding result codes are to be
// either added to the list below
enum class CheckResult: std::int8_t {
  RESULT_VALID = 0,
  RESULT_INVALID,
  RESULT_INVALID_LENGTH,
  RESULT_IS_NULL,
  RESULT_NOT_A_FUNCTION,
  RESULT_NOT_A_STRING,
  RESULT_NOT_AN_ARRAY,
  RESULT_NOT_AN_INT32,
  RESULT_NOT_AN_OBJECT
};

typedef InjectionWrapper::CallbackInfoValue MethodArgsType;
typedef std::function<CheckResult(const MethodArgsType&)> InitFunction;

class MethodArgBinding;

// for getting verbose representation of correspondng checking result codes
const char* GetCheckResultName(CheckResult result);

// The general type used for checking arguments wrapping source parameters,
// checking their amount, providing traversal among them and methods chaining
// (for building consecutive handy call chains).
class CheckArguments {
 public:
  explicit CheckArguments(const MethodArgsType& args);
  CheckArguments(const CheckArguments& args);
  ~CheckArguments();

  // for checking number of arguments
  CheckArguments& ArgumentsCount(int count);

  // for arguments travesal
  MethodArgBinding Argument(int index);

  // for methods chaining (using predicates)
  operator CheckResult() const;
  CheckArguments& AddAndClause(InitFunction right_condition);

 private:
  const MethodArgsType& args_;
  InitFunction init_;
};

// The supplementary type for checking arguments containing parameter type
// checking and binding functionality.
class MethodArgBinding {
 public:
  MethodArgBinding(int index, CheckArguments& parent);

  // type checking functions
  MethodArgBinding& IsArray();
  MethodArgBinding& IsFunction();
  MethodArgBinding& IsInt32();
  MethodArgBinding& IsObject();
  MethodArgBinding& IsString();
  MethodArgBinding& NotNull();
  // and more (if reasonable)

  // value binding functions
  template <typename T>
  CheckArguments& BindLocal(JSLocal<T>& value) {
    return parent_.AddAndClause([this, &value](const MethodArgsType& args) {
      value = JSLocal<T>(args[arg_index_]).As();
      return CheckResult::RESULT_VALID;
    });
  }

  CheckArguments& BindInt32(int32_t& value);
  CheckArguments& BindString(std::string& value);
  // and more (if reasonable)

 private:
  int arg_index_;
  CheckArguments& parent_;
};

}  // namespace extensions_v8

#endif  // INJECTION_COMMON_UTIL_ARGUMENTS_CHECKING_H_
