// Copyright (c) 2019 LG Electronics, Inc.
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

#ifndef NEVA_INJECTION_WEBOSSERVICEBRIDGE_WEBOSSERVICEBRIDGE_EXPORT_H_
#define NEVA_INJECTION_WEBOSSERVICEBRIDGE_WEBOSSERVICEBRIDGE_EXPORT_H_

#if defined(COMPONENT_BUILD)

#if defined(WEBOSSERVICEBRIDGE_IMPLEMENTATION)
#define WEBOSSERVICEBRIDGE_EXPORT __attribute__((visibility("default")))
#else
#define WEBOSSERVICEBRIDGE_EXPORT
#endif  // defined(WEBOSSERVICEBRIDGE_IMPLEMENTATION)

#else
#define WEBOSSERVICEBRIDGE_EXPORT
#endif  // defined(COMPONENT_BUILD)

#endif  // NEVA_INJECTION_WEBOSSERVICEBRIDGE_WEBOSSERVICEBRIDGE_EXPORT_H_
