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

#ifndef NET_CODE_CACHE_DUMMY_CODE_CACHE_H_
#define NET_CODE_CACHE_DUMMY_CODE_CACHE_H_

#include "net/code_cache/code_cache.h"

#include "base/files/file_path.h"
#include "base/threading/thread_checker.h"
#include "net/base/io_buffer.h"
#include "net/base/net_export.h"
#include "net/base/request_priority.h"
#include "url/gurl.h"

#include <memory>
#include <vector>

class GURL;

namespace base {
class SingleThreadTaskRunner;
}

namespace net {

class NET_EXPORT DummyCodeCache : public CodeCache {
 public:
  DummyCodeCache(const base::FilePath& path, int max_size);

  ~DummyCodeCache() override;

  void WriteMetadata(const GURL& url,
                     scoped_refptr<IOBufferWithSize> buf) override;

  void ReadMetadata(const GURL& url,
                    const base::Time& last_modified,
                    ReadCallback& callback) override;

  void ClearData(
      const CompletionCallback& callback = CompletionCallback()) override;

 private:
  base::FilePath path_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(DummyCodeCache);
};

}  // namespace net

#endif  // NET_CODE_CACHE_DUMMY_CODE_CACHE_H_
