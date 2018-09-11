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

#ifndef MEDIA_BASE_NEVA_MIME_UTIL_INTERNAL_H_
#define MEDIA_BASE_NEVA_MIME_UTIL_INTERNAL_H_

#include "media/base/mime_util_internal.h"

namespace media {

namespace internal {

class MEDIA_EXPORT NevaMimeUtil : public MimeUtil {
 public:
  NevaMimeUtil();
  ~NevaMimeUtil();

 private:
  typedef base::flat_set<int> CodecSet;
  void InitializeMimeTypeMaps();
  void AddSupportedMediaFormats();
};

} //namespace internal
} //namespace media

#endif  // MEDIA_BASE_NEVA_MIME_UTIL_INTERNAL_H_
