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

#include "media/blink/neva/video_frame_provider_impl.h"

#include <GLES2/gl2ext.h>

#include "base/command_line.h"
#include "base/logging.h"
#include "content/public/renderer/render_frame.h"
#include "content/renderer/media/neva/stream_texture_factory.h"
#include "content/renderer/render_thread_impl.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_log.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"
#include "third_party/blink/public/platform/web_media_player_client.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/public/web/web_view.h"

#define FUNC_LOG(x) DVLOG(x) << __func__

namespace media {

VideoFrameProviderImpl::VideoFrameProviderImpl(
    const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
    const scoped_refptr<base::SingleThreadTaskRunner>& compositor_task_runner)
    : frame_(nullptr),
      client_(nullptr),
      is_suspended_(false),
      natural_video_size_(gfx::Size(1, 1)),
      video_frame_provider_client_(nullptr),
      main_loop_(base::MessageLoop::current()),
      compositor_task_runner_(compositor_task_runner),
      stream_texture_factory_create_cb_(stream_texture_factory_create_cb),
      texture_id_(0),
      stream_id_(0),
      storage_type_(media::VideoFrame::STORAGE_UNKNOWN) {
  FUNC_LOG(1);
}

VideoFrameProviderImpl::~VideoFrameProviderImpl() {
  FUNC_LOG(1);
  SetVideoFrameProviderClient(nullptr);
  DeleteStreamTexture();
}

void VideoFrameProviderImpl::CreateStreamTextureProxyIfNeeded() {
  FUNC_LOG(1) << " stream_id_:" << stream_id_
              << " stream_texture_proxy_interface_:"
              << stream_texture_proxy_interface_.get()
              << " stream_texture_factory_interface_:"
              << stream_texture_factory_interface_.get();
  // Return if the proxy is already created, create it otherwise
  if (stream_texture_proxy_interface_)
    return;

  // No factory to create proxy.
  if (!stream_texture_factory_interface_)
    return;

  stream_texture_proxy_interface_.reset(
      stream_texture_factory_interface_->CreateProxy());

  if (stream_texture_proxy_interface_ && video_frame_provider_client_) {
    stream_texture_proxy_interface_->BindToLoop(
        stream_id_, video_frame_provider_client_, compositor_task_runner_);
  }
}

void VideoFrameProviderImpl::CreateStreamTexture() {
  FUNC_LOG(1);
  DCHECK(!stream_id_);
  DCHECK(!texture_id_);

  stream_texture_factory_interface_ = stream_texture_factory_create_cb_.Run();
  stream_id_ = stream_texture_factory_interface_->CreateStreamTexture(
      GL_TEXTURE_EXTERNAL_OES, &texture_id_, &texture_mailbox_);

  if (stream_id_)
    CreateStreamTextureProxyIfNeeded();
}

void VideoFrameProviderImpl::DeleteStreamTexture() {
  FUNC_LOG(1);
  if (stream_id_) {
    GLES2Interface* gl = stream_texture_factory_interface_->ContextGL();
    gl->DeleteTextures(1, &texture_id_);
    // Flush to ensure that the stream texture gets deleted in a timely fashion.
    gl->ShallowFlushCHROMIUM();
    texture_id_ = 0;
    texture_mailbox_ = gpu::Mailbox();
    stream_id_ = 0;

    if (stream_texture_proxy_interface_)
      stream_texture_proxy_interface_.reset();

    stream_texture_factory_interface_ = nullptr;
    active_video_region_ = blink::WebRect();
  }
}

// This will be called when VideoFrame holding mailbox_holder destructed.
// Delete texture refering to StreamTexture to recyle StreamTexture
// static
void VideoFrameProviderImpl::OnReleaseTexture(
    const scoped_refptr<media::StreamTextureFactoryInterface>& factories,
    GLuint texture_id,
    const gpu::SyncToken& release_sync_token) {
  FUNC_LOG(1);
  GLES2Interface* gl = factories->ContextGL();
  if (!gl)
    return;
  // wait for other client to use mailbox
  if (release_sync_token.HasData())
    gl->WaitSyncTokenCHROMIUM(release_sync_token.GetConstData());
  gl->DeleteTextures(1, &texture_id);
}

void VideoFrameProviderImpl::ActiveRegionChanged(
    const blink::WebRect& active_region) {
  FUNC_LOG(1) << " active_region:" << gfx::Rect(active_region).ToString();
  if (active_video_region_ != active_region) {
    active_video_region_ = active_region;
    UpdateVideoFrame();
  }
}

void VideoFrameProviderImpl::UpdateVideoFrame() {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  FUNC_LOG(1);

  // TODO : Need to re-implement for set appropriate time to
  // create/destroy stream textures
  if (storage_type_ == media::VideoFrame::STORAGE_OPAQUE && !stream_id_)
    CreateStreamTexture();

  if (is_suspended_ && stream_id_)
    DeleteStreamTexture();

  current_frame_ = CreateVideoFrame(storage_type_);

  Repaint();
}

scoped_refptr<VideoFrame> VideoFrameProviderImpl::CreateVideoFrame(
    media::VideoFrame::StorageType frame_storage_type) {
  FUNC_LOG(1) << " frame_storage_type:" << frame_storage_type;

  switch (frame_storage_type) {
    case media::VideoFrame::STORAGE_OPAQUE: {
      if (!stream_texture_factory_interface_)
        break;
      stream_texture_factory_interface_->SetStreamTextureActiveRegion(
          stream_id_, active_video_region_);

      GLES2Interface* gl = stream_texture_factory_interface_->ContextGL();
      // Add ref count mailbox texture to use StreamTexture after destruction of
      // VFPI see https://codereview.chromium.org/192813003/
      GLuint texture_id_ref = 0;
      gl->GenTextures(1, &texture_id_ref);
      GLuint texture_target = GL_TEXTURE_EXTERNAL_OES;
      gl->BindTexture(texture_target, texture_id_ref);
      gl->CreateAndConsumeTextureCHROMIUM(texture_mailbox_.name);
      gl->Flush();

      gpu::SyncToken texture_mailbox_sync_token;
      gl->GenUnverifiedSyncTokenCHROMIUM(texture_mailbox_sync_token.GetData());
      if (texture_mailbox_sync_token.namespace_id() ==
          gpu::CommandBufferNamespace::IN_PROCESS) {
        // TODO(boliu): Remove this once Android WebView switches to IPC-based
        // command buffer for video.
        GLbyte* sync_tokens[] = {texture_mailbox_sync_token.GetData()};
        gl->VerifySyncTokensCHROMIUM(sync_tokens, arraysize(sync_tokens));
      }

      gpu::MailboxHolder holders[media::VideoFrame::kMaxPlanes] = {
          gpu::MailboxHolder(texture_mailbox_, texture_mailbox_sync_token,
                             texture_target)};
      gfx::Size coded_size(active_video_region_.width,
                           active_video_region_.height);
      gfx::Rect visible_rect(0, 0, active_video_region_.width,
                             active_video_region_.height);

      return media::VideoFrame::WrapNativeTextures(
          media::PIXEL_FORMAT_ARGB, holders,
          media::BindToCurrentLoop(
              base::Bind(&VideoFrameProviderImpl::OnReleaseTexture,
                         stream_texture_factory_interface_, texture_id_ref)),
          coded_size, visible_rect, natural_video_size_, base::TimeDelta());
    }
#if defined(VIDEO_HOLE)
    case media::VideoFrame::STORAGE_HOLE:
      return media::VideoFrame::CreateHoleFrame(natural_video_size_);
#endif
    case media::VideoFrame::STORAGE_BLACK:
      return media::VideoFrame::CreateBlackFrame(natural_video_size_);
    default:
      return nullptr;
  }
  return nullptr;
}

blink::WebMediaPlayerClient* VideoFrameProviderImpl::GetClient() {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  DCHECK(client_);
  return client_;
}

void VideoFrameProviderImpl::SetVideoFrameProviderClient(
    cc::VideoFrameProvider::Client* client) {
  FUNC_LOG(1);
  // This is called from both the main renderer thread and the compositor
  // thread (when the main thread is blocked).
  if (video_frame_provider_client_)
    video_frame_provider_client_->StopUsingProvider();
  video_frame_provider_client_ = client;

  if (stream_texture_proxy_interface_)
    stream_texture_proxy_interface_->BindToLoop(
        stream_id_, video_frame_provider_client_, compositor_task_runner_);
}

void VideoFrameProviderImpl::PutCurrentFrame() {}

scoped_refptr<media::VideoFrame> VideoFrameProviderImpl::GetCurrentFrame() {
  return current_frame_;
}

bool VideoFrameProviderImpl::UpdateCurrentFrame(base::TimeTicks deadline_min,
                                                base::TimeTicks deadline_max) {
  NOTIMPLEMENTED();
  return false;
}

bool VideoFrameProviderImpl::HasCurrentFrame() {
  return current_frame_ != nullptr;
}

void VideoFrameProviderImpl::Repaint() {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  GetClient()->Repaint();
}

void VideoFrameProviderImpl::SetNaturalVideoSize(gfx::Size natural_video_size) {
  FUNC_LOG(1) << " " << natural_video_size.ToString();
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  natural_video_size_ =
      natural_video_size.IsEmpty() ? gfx::Size(1, 1) : natural_video_size;
}

void VideoFrameProviderImpl::SetStorageType(
    media::VideoFrame::StorageType type) {
  FUNC_LOG(1) << " type:" << type;
  if (storage_type_ == type)
    return;

  storage_type_ = type;
  if (storage_type_ != media::VideoFrame::STORAGE_OPAQUE)
    UpdateVideoFrame();
}

void VideoFrameProviderImpl::SetCurrentVideoFrame(
    scoped_refptr<media::VideoFrame> current_frame) {
  current_frame_ = current_frame;
}

base::Lock& VideoFrameProviderImpl::GetLock() {
  return lock_;
}

void VideoFrameProviderImpl::SetWebLocalFrame(blink::WebLocalFrame* frame) {
  frame_ = frame;
}

void VideoFrameProviderImpl::SetWebMediaPlayerClient(
    blink::WebMediaPlayerClient* client) {
  client_ = client;
}
blink::WebRect& VideoFrameProviderImpl::GetActiveVideoRegion() {
  return active_video_region_;
}

gfx::Size VideoFrameProviderImpl::GetNaturalVideoSize() {
  return natural_video_size_;
}

}  // namespace media
