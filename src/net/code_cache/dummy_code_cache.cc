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

#include "net/code_cache/dummy_code_cache.h"

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "net/base/net_errors.h"
#include "net/disk_cache/disk_cache.h"
#include "url/gurl.h"

class IOBufffer;

namespace net {

DummyCodeCache::DummyCodeCache(const base::FilePath& path, int max_size)
    : path_(path) {}

DummyCodeCache::~DummyCodeCache() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  LOG(INFO) << "~DummyCodeCache";
}

void DummyCodeCache::WriteMetadata(const GURL& url,
                                   scoped_refptr<IOBufferWithSize> buf) {}

void DummyCodeCache::ReadMetadata(const GURL& url,
                                  const base::Time& last_modified,
                                  ReadCallback& read_callback) {}

void DummyCodeCache::ClearData(const CompletionCallback& callback) {
  const base::FilePath cache_path = path_.Append("CodeCache");
  bool code_cache_dir_existed = base::PathExists(cache_path);

  if (code_cache_dir_existed) {
    DeleteFile(cache_path, true);
  }
}

}  // namespace net
