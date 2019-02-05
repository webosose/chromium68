// Copyright (c) 2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "neva/app_runtime/browser/app_runtime_quota_permission_context.h"

#include "base/command_line.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_neva_switches.h"
#include "neva/app_runtime/browser/app_runtime_quota_permission_delegate.h"
#include "third_party/blink/public/mojom/quota/quota_types.mojom.h"

namespace app_runtime {

AppRuntimeQuotaPermissionContext::AppRuntimeQuotaPermissionContext(
    AppRuntimeQuotaPermissionDelegate* delegate)
    : delegate_(delegate) {}

void AppRuntimeQuotaPermissionContext::RequestQuotaPermission(
    const content::StorageQuotaParams& params,
    int render_process_id,
    const PermissionCallback& callback) {
  if (params.storage_type != blink::mojom::StorageType::kPersistent) {
    // For now we only support requesting quota with this interface
    // for Persistent storage type.
    callback.Run(QUOTA_PERMISSION_RESPONSE_DISALLOW);
    return;
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableFileAPIDirectoriesAndSystem)) {
    callback.Run(QUOTA_PERMISSION_RESPONSE_ALLOW);
    return;
  }

  bool handled(false);
  if (delegate_)
    handled = delegate_->OnRequestQuotaPermission(params, render_process_id,
                                                  callback);

  if (!handled)
    callback.Run(QUOTA_PERMISSION_RESPONSE_DISALLOW);
}

}  // namespace app_runtime
