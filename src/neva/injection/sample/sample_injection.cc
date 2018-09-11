// Copyright (c) 2016-2017 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.

#include "injection/sample/sample_injection.h"

#include <sstream>

#include "base/bind.h"
#include "base/macros.h"
#include "content/public/renderer/render_frame.h"
#include "injection/common/public/renderer/injection_base.h"
#include "injection/common/util/arguments_checking.h"
#include "injection/common/wrapper/injection_wrapper.h"
#include "pal/ipc/renderer/sample/sample_proxy.h"
#include "pal/public/interfaces/sample_interface.h"
#include "pal/public/pal.h"
#include "pal/public/pal_factory.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace {

// cached value for SampleInjection::GetValue function
static std::string app_value = "null";

}  // anonymous namespace

namespace extensions_v8 {

const char SampleInjectionExtension::kInjectionName[] = "v8/sample";

const char kSampleInjectionAPI[] =
R"JS(
function Sample() {
    if (typeof(navigator.sample) != 'undefined') {
        throw new TypeError("Illegal constructor");
    };

    function init() {
        var _obj = {};

        var gCallbackID = 0;
        var gCallbackObjects = [];

        var registerCallback = function(callback) {
            if (callback instanceof Function) {
                var i = gCallbackID;
                gCallbackObjects[i] = callback;
                gCallbackID++;
                return i;
            }
            console.log("Error: passed argument is not a function");
            return -1;
        };

        var unregisterCallback = function(callbackID) {
            delete gCallbackObjects[callbackID];
        };

        // This is how to reuse the implementation of EventTarget methods from
        // DocumentFragment.
        // To subscribe/unsubscribe to/from a platform event dispatching, native
        // functions implementing the subscription/unsubscription are to be
        // invoked from the EventTarget methods.
        // Wrapper functions mocking some of these EventTarget methods and
        // performing proper native calls are represented below and named as
        // addEventListener and removeEventListener correspondingly.
        var eventTarget = document.createDocumentFragment();
        function createBinding(method) {
            this[method] = eventTarget[method].bind(eventTarget);
        }
        [
            "addEventListener",
            "dispatchEvent",
            "removeEventListener"
        ].forEach(createBinding, _obj);

        // This is how to subscribe to event from native code.
        // For instance:
        //   navigator.sample.addEventListener('samplenotify',eventCallback);
        // where 'eventCallback' is a predefined function.
        var originalAddEventListener = _obj.addEventListener;
        _obj.addEventListener = function(type, listener, useCapture) {
            originalAddEventListener(type, listener, useCapture);
            native function SubscribeToEvent();
            return SubscribeToEvent();
        };

        // This is how to unsubscribe from event from native code.
        // For instance:
        //   navigator.sample.removeEventListener('samplenotify',eventCallback);
        // where 'eventCallback' is a predefined function.
        var originalRemoveEventListener = _obj.removeEventListener;
        _obj.removeEventListener = function(type, listener, useCapture) {
            originalRemoveEventListener(type, listener, useCapture);
            native function UnsubscribeFromEvent();
            return UnsubscribeFromEvent();
        };

        // Here is how to define a property which will return the result of
        // a native function call
        Object.defineProperty(_obj, "value", {
            configurable: false,
            get: function() {
               native function GetValue();
               return GetValue();
            }
        })

        // This function will be called when 'onchange' event is fired
        _obj.onchange = function(event) {};

        // This function will be called when 'onsamplenotify' event is fired
        _obj.onsamplenotify = function(event) {};

        // This function will return the result of a native function call
        _obj.getValue = function() {
            native function GetValue();
            return GetValue();
        };

        // This function will return the result of a native function call
        _obj.getPlatformValue = function() {
            native function GetPlatformValue();
            return GetPlatformValue();
        };

        // This is the way to pass arguments to a native function
        _obj.callFunc = function(arg1, arg2) {
            native function CallFunc(arg1, arg2);
            return CallFunc(arg1, arg2);
        };

        // This function will pass the data to its native function, once
        // corresponding result is obtained, the latter will be returned to the
        // invoked callback function previously passed to the function
        // (multiple callback objects without any identifiers provided version).
        _obj.processData = function(callback, data) {
            native function ProcessData(callbackID, data);
            console.log("called processData");
            console.log("data = " + data);
            var result = registerCallback(callback);
            if (result >= 0) {
                return ProcessData(result, data);
            }
            console.log("failed to call ProcessData");
        };

        // a handler object (invoked from the native layer and intended for
        // a callback lookup by using the callbackID provided).
        // It isn't part of the public Sample injection API.
        _obj.handlerObject_ = function(result, callbackID, data) {
          console.log("called handlerObject_");
          console.log("result = " + result + ", callbackID = " + callbackID +
                      ", data = " + data);
          try {
            gCallbackObjects[callbackID](result, callbackID, data);
            unregisterCallback(callbackID);
          } catch(err) {
            console.log("Error: found no callback object for the callbackID = "
                        + callbackID);
          }
        }
        Object.defineProperty(_obj, "handlerObject_", {
          enumerable: false,
          writable: false,
          configurable: false
        });

        return _obj;
    }

    // It allows to initialize object Sample and get access to it from navigator
    function get() {
        var value = init.call(this);
        Object.defineProperty(this, "sample", {
            value: value,
            configurable: false,
        });
        return value;
    }

    return {
        get: get,
        configurable: true,
    };
};
)JS";

static const char kDispatchValueChangedScript[] =
R"JS(
if (typeof navigator !== 'undefined' && navigator
    && navigator.sample) {
  var event = new Event("change");
  navigator.sample.dispatchEvent(event);
  if (navigator.sample.onchange !== undefined
      && navigator.sample.onchange instanceof Function) {
    navigator.sample.onchange(event);
  }
}
)JS";

static const char kInstallSampleInjection[] =
R"JS(
if (typeof Sample !== 'undefined' && navigator) {
  if (document && (typeof(navigator.sample) == 'undefined')){
    Object.defineProperty(navigator, "sample", Sample());
  };
};
)JS";

class SampleInjection : public InjectionWrapper, public InjectionBase {
 public:
  SampleInjection();

  // "native-functions" list
  // Call function from platform. Return nothing
  static void CallFunc(const InjectionWrapper::CallbackInfoValue& args);
  // Get cached value
  static void GetValue(const InjectionWrapper::CallbackInfoValue& args);
  // Get platform value
  static void GetPlatformValue(const InjectionWrapper::CallbackInfoValue& args);
  // Process data received from app (multiple callback objects case)
  static void ProcessData(const InjectionWrapper::CallbackInfoValue& args);
  // Subscribe "sample" object to literal data notifications
  static void SubscribeToEvent(const InjectionWrapper::CallbackInfoValue& args);
  // Unsubscribe "sample" object from literal data notifications
  static void UnsubscribeFromEvent(
      const InjectionWrapper::CallbackInfoValue& args);

  static void OnProcessDataRespond(blink::WebLocalFrame* web_frame,
                                   int32_t callback_id, const std::string& data,
                                   bool result);

 private:
  static void ReceivedSampleUpdate(blink::WebLocalFrame* web_frame,
      const std::string& value);

  static pal::SampleInterface::SampleUpdateCallback sampleUpdateCallback_;
  static std::unique_ptr<pal::SampleInterface::SampleUpdateSubscription>
      sampleUpdateSubscription_;
};

pal::SampleInterface::SampleUpdateCallback
    SampleInjection::sampleUpdateCallback_;
std::unique_ptr<pal::SampleInterface::SampleUpdateSubscription>
    SampleInjection::sampleUpdateSubscription_;

SampleInjection::SampleInjection()
    : InjectionWrapper(SampleInjectionExtension::kInjectionName,
                       kSampleInjectionAPI) {
  IW_ADDCALLBACK(CallFunc);
  IW_ADDCALLBACK(GetValue);
  IW_ADDCALLBACK(GetPlatformValue);
  IW_ADDCALLBACK(ProcessData);
  IW_ADDCALLBACK(SubscribeToEvent);
  IW_ADDCALLBACK(UnsubscribeFromEvent);
}

// static
void SampleInjection::CallFunc(
    const InjectionWrapper::CallbackInfoValue& args) {
  content::RenderFrame* frame = content::RenderFrame::FromWebFrame(
      blink::WebLocalFrame::FrameForCurrentContext());
  if (!frame)
    return;

  std::string arg_1, arg_2;
  CheckResult checking_result;

  if ((checking_result = CheckArguments(args).ArgumentsCount(2)
      .Argument(0).IsString().BindString(arg_1)
      .Argument(1).IsString().BindString(arg_2)) == CheckResult::RESULT_VALID) {
    pal::SampleInterface* interface = pal::GetInstance()->GetSampleInterface();

    if (interface == nullptr) {
      LOG(ERROR) << __func__
          << "(): no remote PAL implementation for the interface";
      return;
    }

    interface->CallFunc(arg_1, arg_2);
  } else {
    LOG(ERROR) << __func__ << "(): arguments checking failed: "
        << GetCheckResultName(checking_result);
  }
}

// static
void SampleInjection::GetValue(
    const InjectionWrapper::CallbackInfoValue& args) {
  LOG(INFO) << __func__ << "(): called";
  JSFunctionCallbackInfoValue js_args(args);

  JSReturnValue ret_val(js_args.GetReturnValue());
  ret_val.Set(
      InjectionWrapper::NewLocalStringFromUtf8(js_args.GetIsolate(),
                                               app_value.c_str()));
}

// static
void SampleInjection::GetPlatformValue(
    const InjectionWrapper::CallbackInfoValue& args) {
  LOG(INFO) << __func__ << "(): called";
  JSFunctionCallbackInfoValue js_args(args);

  pal::SampleInterface* interface = pal::GetInstance()->GetSampleInterface();

  if (interface == nullptr) {
    LOG(ERROR) << __func__ << "(): No PROXY implemented for interface";
    return;
  }
  std::string pal_ret;
  pal_ret = interface->GetPlatformValue();

  JSReturnValue ret_val(js_args.GetReturnValue());
  ret_val.Set(
      InjectionWrapper::NewLocalStringFromUtf8(js_args.GetIsolate(),
                                               pal_ret.c_str()));
}

//static
void SampleInjection::OnProcessDataRespond(blink::WebLocalFrame* web_frame,
    int32_t callback_id, const std::string& data, bool result) {
  LOG(INFO) << __func__ << "(): web_frame = " << web_frame << ", callback_id = "
      << callback_id << ", data = " << data << ", result = " << result;

  std::stringstream script;

  script << "if(typeof navigator !== 'undefined' && navigator && "
      "navigator.sample) {navigator.sample.handlerObject_(" <<
      (result ? "true" : "false") << "," << std::to_string(callback_id) <<
      ",'" << data << "');}";

  SampleInjection::Dispatch(web_frame,
                            blink::WebString::FromUTF8(script.str().c_str()));
}

// static
void SampleInjection::ProcessData(
    const InjectionWrapper::CallbackInfoValue& args) {
  LOG(INFO) << __func__ << "(): called";

  blink::WebLocalFrame* web_frame =
      blink::WebLocalFrame::FrameForCurrentContext();

  if (!web_frame) {
    LOG(ERROR) << __func__ << "(): invalid web_frame";
    return;
  }

  int32_t callback_id = 0;
  std::string data;
  CheckResult checking_result;

  if ((checking_result = CheckArguments(args).ArgumentsCount(2)
      .Argument(0).IsInt32().BindInt32(callback_id)
      .Argument(1).IsString().BindString(data)) == CheckResult::RESULT_VALID) {
    LOG(INFO) << __func__ << "(): web_frame = " << web_frame
        << ", callback_id = " << callback_id << ", data = " << data;

    pal::SampleInterface* interface = pal::GetInstance()->GetSampleInterface();

    if (interface == nullptr) {
      LOG(ERROR) << __func__
          << "(): no remote PAL implementation for the interface";
      return;
    }

    interface->ProcessData(data,
                           base::Bind(&SampleInjection::OnProcessDataRespond,
                                      web_frame, callback_id, data));
  } else {
    LOG(ERROR) << __func__ << "(): arguments checking failed: "
        << GetCheckResultName(checking_result);
  }
}

// static
void SampleInjection::SubscribeToEvent(
    const InjectionWrapper::CallbackInfoValue& args) {
  LOG(INFO) << __func__ << "(): called";

  blink::WebLocalFrame* web_frame =
      blink::WebLocalFrame::FrameForCurrentContext();

  if (!web_frame) {
    LOG(ERROR) << __func__ << "(): invalid web_frame";
    return;
  }

  pal::SampleInterface* interface = pal::GetInstance()->GetSampleInterface();

  if (interface == nullptr) {
    LOG(ERROR) << __func__
        << "(): no remote PAL implementation for the interface";
    return;
  }

  if (!sampleUpdateCallback_) {
    sampleUpdateCallback_ = base::Bind(&SampleInjection::ReceivedSampleUpdate,
                                       web_frame);
  }

  if (!sampleUpdateSubscription_.get()) {
    sampleUpdateSubscription_ = interface->AddCallback(sampleUpdateCallback_);
    LOG(INFO) << __func__ << "(): successfully subscribed";
  }
}

// static
void SampleInjection::UnsubscribeFromEvent(
    const InjectionWrapper::CallbackInfoValue& args) {
  LOG(INFO) << __func__ << "(): called";

  if (sampleUpdateSubscription_.get()) {
    sampleUpdateSubscription_.reset();
    LOG(INFO) << __func__ << "(): successfully unsubscribed";
  }
}

// static
InjectionWrapper* SampleInjectionExtension::Get() {
  return new SampleInjection();
}

// static
void SampleInjection::ReceivedSampleUpdate(blink::WebLocalFrame* web_frame,
    const std::string& value) {
  LOG(INFO) << __func__ << "(): web_frame = " << web_frame << ", value = "
      << value;

  std::stringstream script;

  script << "if(typeof navigator !== 'undefined' && navigator && "
      "navigator.sample) {var event = new CustomEvent('samplenotify', "
      "{detail: {value: '" << value << "'}});"
      "navigator.sample.dispatchEvent(event);"
      "if(navigator.sample.onsamplenotify !== undefined && "
      "navigator.sample.onsamplenotify instanceof Function) {"
      "navigator.sample.onsamplenotify(event);}}";

  SampleInjection::Dispatch(web_frame,
                            blink::WebString::FromUTF8(script.str().c_str()));
}

// static
void SampleInjectionExtension::DispatchValueChanged(
    blink::WebLocalFrame* web_frame,
    const std::string& val) {
  if (app_value == val) {
    return;
  }

  app_value = val;

  if (web_frame) {
    SampleInjection::Dispatch(web_frame, kDispatchValueChangedScript);
  }
}

// static
void SampleInjectionExtension::InstallExtension(
    blink::WebLocalFrame* web_frame) {
  if (web_frame) {
    SampleInjection::Dispatch(web_frame, kInstallSampleInjection);
  }
}

std::string SampleInjectionExtension::GetNavigatorExtensionScript() {
  return kInstallSampleInjection;
}

}  // namespace extensions_v8
