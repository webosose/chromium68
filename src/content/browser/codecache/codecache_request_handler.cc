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

#include "content/browser/codecache/codecache_request_handler.h"

#include "base/files/file_path.h"
#include "base/task_scheduler/post_task.h"
#include "content/browser/codecache/codecache_url_request_job.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/filename_util.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_job.h"

namespace content {

CodeCacheRequestHandler::CodeCacheRequestHandler()
    : request_job_delivered_(false) {}

CodeCacheURLRequestJob* CodeCacheRequestHandler::MaybeLoadResource(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) {
  if (request_job_delivered_)
    return nullptr;

  base::FilePath file_path;
  const bool is_file = net::FileURLToFilePath(request->url(), &file_path);
  if (!is_file)
    return nullptr;

  // It will be destroied after the job finished.
  CodeCacheURLRequestJob* job = new CodeCacheURLRequestJob(
      request, network_delegate, file_path,
      base::CreateTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN}));
  request_job_delivered_ = true;
  return job;
}

}  // namespace content
