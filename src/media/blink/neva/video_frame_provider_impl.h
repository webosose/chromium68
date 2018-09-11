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

#ifndef MEDIA_BLINK_NEVA_VIDEO_FRAME_PROVIDER_IMPL_H_
#define MEDIA_BLINK_NEVA_VIDEO_FRAME_PROVIDER_IMPL_H_

#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "cc/layers/video_frame_provider.h"
#include "content/renderer/media/neva/stream_texture_factory.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "media/base/pipeline_metadata.h"
#include "media/base/renderer_factory.h"
#include "media/base/video_frame.h"
#include "media/blink/media_blink_export.h"
#include "media/blink/neva/stream_texture_interface.h"
#include "media/blink/webmediaplayer_delegate.h"
#include "media/blink/webmediaplayer_params.h"
#include "third_party/blink/public/platform/web_media_player.h"
#include "third_party/blink/public/platform/web_rect.h"

namespace blink {
class WebLocalFrame;
class WebMediaPlayerClient;
class WebMediaPlayerEncryptedMediaClient;
}  // namespace blink

namespace content {
class RenderThreadImpl;
class StreamTextureFactory;
}  // namespace content

namespace gpu {
struct SyncToken;
}

using gpu::gles2::GLES2Interface;

namespace media {

// This class provides three types of VideoFrame
//  STORAGE_HOLE   : support video hole based media player
//  STORAGE_BLACK  : alter videoframe when unavailable from media player
//  STORAGE_OPAQUE : support video texture
class MEDIA_BLINK_EXPORT VideoFrameProviderImpl
    : public cc::VideoFrameProvider,
      public base::SupportsWeakPtr<VideoFrameProviderImpl> {
 public:
  VideoFrameProviderImpl(
      const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
      const scoped_refptr<base::SingleThreadTaskRunner>&
          compositor_task_runner);

  ~VideoFrameProviderImpl() override;

  // cc::VideoFrameProvider implementation.
  void SetVideoFrameProviderClient(
      cc::VideoFrameProvider::Client* client) override;
  scoped_refptr<media::VideoFrame> GetCurrentFrame() override;
  void PutCurrentFrame() override;
  bool UpdateCurrentFrame(base::TimeTicks deadline_min,
                          base::TimeTicks deadline_max) override;
  bool HasCurrentFrame() override;

  // Getter method to |client_|.
  blink::WebMediaPlayerClient* GetClient();
  void ActiveRegionChanged(const blink::WebRect&);
  void Repaint();
  void SetCurrentVideoFrame(scoped_refptr<media::VideoFrame> current_frame);
  base::Lock& GetLock();
  void SetWebLocalFrame(blink::WebLocalFrame* frame);
  void SetWebMediaPlayerClient(blink::WebMediaPlayerClient* client);
  void UpdateVideoFrame();
  blink::WebRect& GetActiveVideoRegion();
  gfx::Size GetNaturalVideoSize();
  void SetNaturalVideoSize(gfx::Size natural_video_size);
  void SetStorageType(media::VideoFrame::StorageType type);

 protected:
  scoped_refptr<VideoFrame> CreateVideoFrame(
      media::VideoFrame::StorageType frame_storage_type);

  void CreateStreamTexture();
  void DeleteStreamTexture();
  void CreateStreamTextureProxyIfNeeded();

  static void OnReleaseTexture(
      const scoped_refptr<media::StreamTextureFactoryInterface>& factories,
      unsigned int texture_id,
      const gpu::SyncToken& release_sync_token);

  blink::WebLocalFrame* frame_;
  blink::WebMediaPlayerClient* client_;
  bool is_suspended_;

  gfx::Size natural_video_size_;

  // Video frame rendering members.
  //
  // |lock_| protects |current_frame_| since new frames arrive on the video
  // rendering thread, yet are accessed for rendering on either the main thread
  // or compositing thread depending on whether accelerated compositing is used.
  base::Lock lock_;
  scoped_refptr<media::VideoFrame> current_frame_;

  // A pointer back to the compositor to inform it about state changes. This is
  // not NULL while the compositor is actively using this webmediaplayer.
  cc::VideoFrameProvider::Client* video_frame_provider_client_;

  // Message loops for posting tasks on Chrome's main thread. Also used
  // for DCHECKs so methods calls won't execute in the wrong thread.
  base::MessageLoop* main_loop_;

  blink::WebRect active_video_region_;

  const scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;

  StreamTextureFactoryCreateCB stream_texture_factory_create_cb_;
  // Object for allocating stream textures
  scoped_refptr<media::StreamTextureFactoryInterface>
      stream_texture_factory_interface_;

  // Object for calling back the compositor thread to repaint the video when a
  // frame available. It should be initialized on the compositor thread.
  // Accessed on main thread and on compositor thread when main thread is
  // blocked.
  media::ScopedStreamTextureProxy stream_texture_proxy_interface_;

  // GL texture ID allocated to the video.
  unsigned int texture_id_;

  // GL texture mailbox for texture_id_ to provide in the VideoFrame, and sync
  // point for when the mailbox was produced.
  gpu::Mailbox texture_mailbox_;

  // Stream texture ID allocated to the video.
  unsigned int stream_id_;

 private:
  media::VideoFrame::StorageType storage_type_;

  DISALLOW_COPY_AND_ASSIGN(VideoFrameProviderImpl);
};

}  // namespace media

#endif  // MEDIA_BLINK_NEVA_VIDEO_FRAME_PROVIDER_IMPL_H_
