// Copyright (c) 2016-2018 LG Electronics, Inc.
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

// Get basic type definitions.
#define IPC_MESSAGE_IMPL
#include "neva/app_runtime/common/app_runtime_message_generator.h"

// Generate constructors.
#include "ipc/struct_constructor_macros.h"
#include "neva/app_runtime/common/app_runtime_message_generator.h"

// Generate destructors.
#include "ipc/struct_destructor_macros.h"
#include "neva/app_runtime/common/app_runtime_message_generator.h"

// Generate param traits write methods.
#include "ipc/param_traits_write_macros.h"
namespace IPC {
#include "neva/app_runtime/common/app_runtime_message_generator.h"
}  // namespace IPC

// Generate param traits read methods.
#include "ipc/param_traits_read_macros.h"
namespace IPC {
#include "neva/app_runtime/common/app_runtime_message_generator.h"
}  // namespace IPC

// Generate param traits log methods.
#include "ipc/param_traits_log_macros.h"
namespace IPC {
#include "neva/app_runtime/common/app_runtime_message_generator.h"
}  // namespace IPC
