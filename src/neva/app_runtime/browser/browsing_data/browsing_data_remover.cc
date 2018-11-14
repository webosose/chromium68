// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#include "browser/browsing_data/browsing_data_remover.h"

#include "base/message_loop/message_loop.h"
#include "components/web_cache/browser/web_cache_manager.h"
#include "content/browser/browsing_data/storage_partition_code_cache_data_remover.h"
#include "content/browser/browsing_data/storage_partition_http_cache_data_remover.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "net/ssl/channel_id_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "neva/app_runtime/browser/app_runtime_browser_context.h"
#include "url/gurl.h"

namespace app_runtime {

// static
BrowsingDataRemover::TimeRange BrowsingDataRemover::Unbounded() {
  return TimeRange(base::Time(), base::Time::Max());
}

BrowsingDataRemover* BrowsingDataRemover::GetForBrowserContext(
    AppRuntimeBrowserContext* browser_context) {
  return new BrowsingDataRemover(browser_context);
}

BrowsingDataRemover::BrowsingDataRemover(
    AppRuntimeBrowserContext* browser_context)
    : browser_context_(browser_context),
      waiting_for_clear_channel_ids_(false),
      waiting_for_clear_cache_(false),
      waiting_for_clear_code_cache_(false),
      waiting_for_clear_storage_partition_data_(false),
      weak_ptr_factory_(this) {}

BrowsingDataRemover::~BrowsingDataRemover() {}

void OnClearedChannelIDsOnIOThread(net::URLRequestContextGetter* rq_context,
                                   const base::Closure& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  rq_context->GetURLRequestContext()
      ->ssl_config_service()
      ->NotifySSLConfigChange();
  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                   callback);
}

void ClearChannelIDsOnIOThread(
    scoped_refptr<net::URLRequestContextGetter> rq_context,
    const base::Closure& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  net::ChannelIDService* channel_id_service =
      rq_context->GetURLRequestContext()->channel_id_service();
  channel_id_service->GetChannelIDStore()->DeleteAll(
      base::Bind(&OnClearedChannelIDsOnIOThread,
                 base::RetainedRef(std::move(rq_context)), callback));
}

void BrowsingDataRemover::Remove(const TimeRange& time_range, int remove_mask) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_NE(base::Time(), time_range.end);

  // Start time to delete from.
  base::Time delete_begin = time_range.begin;

  // End time to delete to.
  base::Time delete_end = time_range.end;

  // TODO (jani) REMOVE_HISTORY support this?
  // Currently we don't track this in WebView

  // TODO (jani) REMOVE_DOWNLOADS currently we don't provide DownloadManager
  // for webview yet

  // Channel IDs are not separated for protected and unprotected web
  // origins. We check the origin_type_mask_ to prevent unintended deletion.
  if (remove_mask & REMOVE_CHANNEL_IDS) {
    // Since we are running on the UI thread don't call GetURLRequestContext().
    scoped_refptr<net::URLRequestContextGetter> rq_context =
        content::BrowserContext::GetDefaultStoragePartition(browser_context_)
            ->GetURLRequestContext();
    waiting_for_clear_channel_ids_ = true;
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::Bind(&ClearChannelIDsOnIOThread, std::move(rq_context),
                   base::Bind(&BrowsingDataRemover::OnClearedChannelIDs,
                              weak_ptr_factory_.GetWeakPtr())));
  }

  uint32_t storage_partition_remove_mask = 0;
  if (remove_mask & REMOVE_COOKIES) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_COOKIES;
  }
  if (remove_mask & REMOVE_LOCAL_STORAGE) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_LOCAL_STORAGE;
  }
  if (remove_mask & REMOVE_INDEXEDDB) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_INDEXEDDB;
  }
  if (remove_mask & REMOVE_WEBSQL) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_WEBSQL;
  }
  if (remove_mask & REMOVE_APPCACHE) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_APPCACHE;
  }
  if (remove_mask & REMOVE_SERVICE_WORKERS) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_SERVICE_WORKERS;
  }
  if (remove_mask & REMOVE_CACHE_STORAGE) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_CACHE_STORAGE;
  }
  if (remove_mask & REMOVE_FILE_SYSTEMS) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_FILE_SYSTEMS;
  }

  if (remove_mask & REMOVE_PLUGIN_DATA) {
    // TODO (jani) Do we need this?
  }

  if (remove_mask & REMOVE_CACHE) {
    // Tell the renderers to clear their cache.
    web_cache::WebCacheManager::GetInstance()->ClearCache();

    waiting_for_clear_cache_ = true;
    // StoragePartitionHttpCacheDataRemover deletes itself when it is done.
    content::StoragePartitionHttpCacheDataRemover::CreateForRange(
        content::BrowserContext::GetDefaultStoragePartition(browser_context_),
        delete_begin, delete_end)
        ->Remove(base::Bind(&BrowsingDataRemover::OnClearedCache,
                            weak_ptr_factory_.GetWeakPtr()));

    // Tell the shader disk cache to clear.
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_SHADER_CACHE;
  }

  // Content Decryption Modules used by Encrypted Media store licenses in a
  // private filesystem. These are different than content licenses used by
  // Flash (which are deleted father down in this method).
  if (remove_mask & REMOVE_MEDIA_LICENSES) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_PLUGIN_PRIVATE_DATA;
  }

  if (remove_mask & REMOVE_CODE_CACHE) {
    waiting_for_clear_code_cache_ = true;
    // StoragePartitionCodeCacheDataRemover deletes itself when it is done.
    content::StoragePartitionCodeCacheDataRemover::CreateForRange(
        content::BrowserContext::GetDefaultStoragePartition(browser_context_),
        delete_begin, delete_end)
        ->Remove(base::Bind(&BrowsingDataRemover::OnClearedCodeCache,
                            weak_ptr_factory_.GetWeakPtr()));
  }

  if (storage_partition_remove_mask) {
    waiting_for_clear_storage_partition_data_ = true;

    content::StoragePartition* storage_partition =
        content::BrowserContext::GetDefaultStoragePartition(browser_context_);

    const uint32_t quota_storage_remove_mask = 0xFFFFFFFF;
    storage_partition->ClearData(
        storage_partition_remove_mask, quota_storage_remove_mask, GURL(),
        content::StoragePartition::OriginMatcherFunction(), delete_begin,
        delete_end,
        base::Bind(&BrowsingDataRemover::OnClearedStoragePartitionData,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

void BrowsingDataRemover::NotifyAndDelete() {
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

void BrowsingDataRemover::NotifyAndDeleteIfDone() {
  if (!AllDone())
    return;

  NotifyAndDelete();
}

void BrowsingDataRemover::OnClearedChannelIDs() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  waiting_for_clear_channel_ids_ = false;
  NotifyAndDeleteIfDone();
}

void BrowsingDataRemover::OnClearedCache() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  waiting_for_clear_cache_ = false;
  NotifyAndDeleteIfDone();
}

void BrowsingDataRemover::OnClearedCodeCache() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  waiting_for_clear_code_cache_ = false;
  NotifyAndDeleteIfDone();
}

void BrowsingDataRemover::OnClearedStoragePartitionData() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  waiting_for_clear_storage_partition_data_ = false;
  NotifyAndDeleteIfDone();
}

bool BrowsingDataRemover::AllDone() {
  return !waiting_for_clear_channel_ids_ && !waiting_for_clear_cache_ &&
         !waiting_for_clear_code_cache_ &&
         !waiting_for_clear_storage_partition_data_;
}

}  // namespace app_runtime
