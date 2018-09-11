// Copyright (c) 2018 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.

#include "injection/memorymanager/memorymanager_injection.h"

#include <sstream>

#include "base/bind.h"
#include "base/macros.h"
#include "content/public/renderer/render_frame.h"
#include "injection/common/public/renderer/injection_base.h"
#include "injection/common/util/arguments_checking.h"
#include "injection/common/wrapper/injection_wrapper.h"
#include "pal/ipc/renderer/memory_manager/memory_manager_proxy.h"
#include "pal/public/interfaces/memory_manager_interface.h"
#include "pal/public/pal.h"
#include "pal/public/pal_factory.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace extensions_v8 {

const char MemoryManagerInjectionExtension::kInjectionName[] = "v8/memorymanager";

const char kMemoryManagerInjectionAPI[] =
R"JS(
function MemoryManager() {
    if (typeof(navigator.memorymanager) != 'undefined') {
        throw new TypeError("Illegal constructor");
    };

    function init() {
        var _obj = {};

        var gCallbackID = 0;
        var gCallbackObjects = [];

        var callOnLevelChangedCallback = function (level) {};

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

        _obj.onlevelchanged = null;

        _obj.getMemoryStatus = function(callback) {
            var callbackID = registerCallback(callback);
            if (callbackID >= 0) {
                native function GetMemoryStatus(callbackID);
                GetMemoryStatus(callbackID);
            }
        };

        _obj.handlerObject_ = function(result, callbackID, data) {
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

        _obj.subscribeToLevelChanged_ = function() {
            native function SubscribeToLevelChanged();
            SubscribeToLevelChanged();
        }

        Object.defineProperty(_obj, "subscribeToLevelChanged_", {
          enumerable: false,
          writable: false,
          configurable: false
        });

        return _obj;
    }

    function get() {
        var value = init.call(this);
        Object.defineProperty(this, "memorymanager", {
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

static const char kInstallMemoryManagerInjection[] =
R"JS(
if (typeof MemoryManager !== 'undefined' && navigator) {
  if (document && (typeof(navigator.memorymanager) == 'undefined')){
    Object.defineProperty(navigator, "memorymanager", MemoryManager());
    navigator.memorymanager.subscribeToLevelChanged_();
  };
};
)JS";

class MemoryManagerInjection : public InjectionWrapper, public InjectionBase {
 public:
  MemoryManagerInjection();


  // "native-functions" list
  static void SubscribeToLevelChanged(
      const InjectionWrapper::CallbackInfoValue& args);
  static void GetMemoryStatus(
      const InjectionWrapper::CallbackInfoValue& args);

 private:
  static void OnLevelChanged(blink::WebLocalFrame* web_frame,
                             const std::string& value);
  static void OnGetMemoryStatusRespond(blink::WebLocalFrame* web_frame,
                                       int32_t callback_id,
                                       const std::string& data);

  static pal::MemoryManagerInterface::LevelChangedCallback
      levelchanged_callback_;
  static std::unique_ptr<pal::MemoryManagerInterface::LevelChangedSubscription>
      levelchanged_subscription_;
};

pal::MemoryManagerInterface::LevelChangedCallback
    MemoryManagerInjection::levelchanged_callback_;
std::unique_ptr<pal::MemoryManagerInterface::LevelChangedSubscription>
    MemoryManagerInjection::levelchanged_subscription_;

MemoryManagerInjection::MemoryManagerInjection()
    : InjectionWrapper(MemoryManagerInjectionExtension::kInjectionName,
                       kMemoryManagerInjectionAPI) {
  IW_ADDCALLBACK(SubscribeToLevelChanged);
  IW_ADDCALLBACK(GetMemoryStatus);
}

// static
void MemoryManagerInjection::SubscribeToLevelChanged(
    const InjectionWrapper::CallbackInfoValue& args) {
  blink::WebLocalFrame* web_frame =
      blink::WebLocalFrame::FrameForCurrentContext();

  pal::MemoryManagerInterface* interface =
      pal::GetInstance()->GetMemoryManagerInterface();

  if (interface == nullptr) {
    LOG(ERROR) << __func__
        << "(): no remote PAL implementation for the interface";
    return;
  }

  if (!levelchanged_callback_) {
    levelchanged_callback_ =
        base::Bind(&MemoryManagerInjection::OnLevelChanged, web_frame);
  }

  if (!levelchanged_subscription_.get())
    levelchanged_subscription_ = interface->AddCallback(levelchanged_callback_);
}

// static
void MemoryManagerInjection::GetMemoryStatus(
    const InjectionWrapper::CallbackInfoValue& args) {
  blink::WebLocalFrame* web_frame =
      blink::WebLocalFrame::FrameForCurrentContext();

  if (!web_frame) {
    LOG(ERROR) << __func__ << "(): invalid web_frame";
    return;
  }

  int32_t callback_id = -1;
  std::string data;
  const CheckResult checking_result = CheckArguments(args).ArgumentsCount(1)
    .Argument(0).IsInt32().BindInt32(callback_id);

  if (checking_result == CheckResult::RESULT_VALID) {
    pal::MemoryManagerInterface* interface =
        pal::GetInstance()->GetMemoryManagerInterface();

    if (interface == nullptr) {
      LOG(ERROR) << __func__
          << "(): no remote PAL implementation for the interface";
      return;
    }

    interface->GetMemoryStatus(
        base::Bind(&MemoryManagerInjection::OnGetMemoryStatusRespond,
                   web_frame, callback_id));
  }
}

// static
InjectionWrapper* MemoryManagerInjectionExtension::Get() {
  return new MemoryManagerInjection();
}

// static
void MemoryManagerInjection::OnLevelChanged(blink::WebLocalFrame* web_frame,
                                            const std::string& value) {
  if (!web_frame)
    return;

  std::stringstream script;
  script << R"JS(
  ;if (typeof navigator !== 'undefined' && navigator && navigator.memorymanager) {
    if (navigator.memorymanager.onlevelchanged != undefined &&
        navigator.memorymanager.onlevelchanged !== null) {
      if (navigator.memorymanager.onlevelchanged instanceof Function)
        navigator.memorymanager.onlevelchanged()JS" << value << R"JS();
    }
  };)JS";

  MemoryManagerInjection::Dispatch(
    web_frame, blink::WebString::FromUTF8(script.str().c_str()));
}

// static
void MemoryManagerInjection::OnGetMemoryStatusRespond(blink::WebLocalFrame* web_frame,
    int32_t callback_id, const std::string& status) {
  if (!web_frame)
    return;

  std::stringstream script;
  script << R"JS(
  ;if(typeof navigator !== 'undefined' && navigator && navigator.memorymanager) {
      navigator.memorymanager.handlerObject_()JS"
          << status << "," << callback_id << R"JS();
  };)JS";

  MemoryManagerInjection::Dispatch(
      web_frame, blink::WebString::FromUTF8(script.str().c_str()));
}

// static
void MemoryManagerInjectionExtension::InstallExtension(
    blink::WebLocalFrame* web_frame) {
  if (web_frame)
    MemoryManagerInjection::Dispatch(web_frame, kInstallMemoryManagerInjection);
}

std::string MemoryManagerInjectionExtension::GetNavigatorExtensionScript() {
  return kInstallMemoryManagerInjection;
}

}  // namespace extensions_v8
