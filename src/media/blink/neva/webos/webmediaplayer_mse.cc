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

#include "media/blink/neva/webos/webmediaplayer_mse.h"

#include "base/command_line.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "cc/layers/layer.h"
#include "cc/layers/video_layer.h"
#include "media/audio/null_audio_sink.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/neva/video_util_neva.h"
#include "media/base/neva/webos/media_platform_api_webos.h"
#include "media/base/renderer_factory_selector.h"
#include "media/blink/webaudiosourceprovider_impl.h"
#include "media/blink/webcontentdecryptionmodule_impl.h"
#include "third_party/blink/public/platform/web_media_player_client.h"
#include "ui/gfx/geometry/rect_f.h"

namespace media {

namespace {

static const int64_t kMinVideoHoleUpdateInterval = 100;

}  // namespace

#define BIND_TO_RENDER_LOOP_VIDEO_FRAME_PROVIDER(function) \
  (DCHECK(main_task_runner_->BelongsToCurrentThread()),    \
   BindToCurrentLoop(                                      \
       base::Bind(function, (this->video_frame_provider_->AsWeakPtr()))))

#define BIND_TO_RENDER_LOOP(function)                   \
  (DCHECK(main_task_runner_->BelongsToCurrentThread()), \
   BindToCurrentLoop(base::Bind(function, base::AsWeakPtr(this))))

WebMediaPlayerMSE::WebMediaPlayerMSE(
    blink::WebLocalFrame* frame,
    blink::WebMediaPlayerClient* client,
    blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
    media::WebMediaPlayerDelegate* delegate,
    std::unique_ptr<media::RendererFactorySelector> renderer_factory_selector,
    UrlIndex* url_index,
    std::unique_ptr<VideoFrameCompositor> compositor,
    const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
    std::unique_ptr<media::WebMediaPlayerParams> params,
    const blink::WebFloatPoint additional_contents_scale,
    const blink::WebString& app_id)
    : media::WebMediaPlayerImpl(frame,
                                client,
                                encrypted_client,
                                delegate,
                                std::move(renderer_factory_selector),
                                url_index,
                                std::move(compositor),
                                std::move(params)),
      additional_contents_scale_(additional_contents_scale),
      app_id_(app_id.Utf8()),
      status_on_suspended_(UnknownStatus),
      is_suspended_(false),
      is_video_offscreen_(false),
      is_fullscreen_(false) {
  // Use the null sink for our MSE player
  audio_source_provider_ = new media::WebAudioSourceProviderImpl(
      new media::NullAudioSink(media_task_runner_), media_log_.get());

  video_frame_provider_ = std::make_unique<VideoFrameProviderImpl>(
      stream_texture_factory_create_cb, vfc_task_runner_);
  video_frame_provider_->SetWebLocalFrame(frame);
  video_frame_provider_->SetWebMediaPlayerClient(client);

  // Create MediaAPIs Wrapper
  media_platform_api_ = media::MediaPlatformAPIWebOS::Create(
      media_task_runner_, client_->IsVideo(), app_id_,
      BIND_TO_RENDER_LOOP_VIDEO_FRAME_PROVIDER(
          &VideoFrameProviderImpl::ActiveRegionChanged),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerMSE::OnError));

  renderer_factory_selector_->GetCurrentFactory()->SetMediaPlatformAPI(
      media_platform_api_);
  pipeline_controller_.SetMediaPlatformAPI(media_platform_api_);

#if defined(ENABLE_LG_SVP)
// TODO(neva): params cannot be used here.
// if (params.initial_cdm()) {
//  const std::string ks =
//      media::ToWebContentDecryptionModuleImpl(params.initial_cdm())
//          ->GetKeySystem();
//  DEBUG_LOG("Setting key_system to media APIs = '%s'", ks.c_str());
//  media_platform_api_->setKeySystem(ks);
//}
#endif

  SetRenderMode(client_->RenderMode());
}

WebMediaPlayerMSE::~WebMediaPlayerMSE() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (video_layer_)
    video_layer_->StopUsingProvider();

  vfc_task_runner_->DeleteSoon(FROM_HERE, std::move(video_frame_provider_));

  if (throttleUpdateVideoHoleBoundary_.IsRunning())
    throttleUpdateVideoHoleBoundary_.Stop();

  if (media_platform_api_.get())
    media_platform_api_->Finalize();
}

void WebMediaPlayerMSE::Load(LoadType load_type,
                             const blink::WebMediaPlayerSource& source,
                             CORSMode cors_mode) {
  // call base-class implementation
  media::WebMediaPlayerImpl::Load(load_type, source, cors_mode);
}

void WebMediaPlayerMSE::Play() {
  DVLOG(1) << __func__ << "play()";
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (is_suspended_) {
    status_on_suspended_ = PlayingStatus;
    return;
  }
  media::WebMediaPlayerImpl::Play();
}

void WebMediaPlayerMSE::Pause() {
  DVLOG(1) << __func__ << "pause()";
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (is_suspended_) {
    status_on_suspended_ = PausedStatus;
    return;
  }
  media::WebMediaPlayerImpl::Pause();
}

void WebMediaPlayerMSE::SetRate(double rate) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (is_suspended_)
    return;
  media::WebMediaPlayerImpl::SetRate(rate);
}

void WebMediaPlayerMSE::SetVolume(double volume) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  media::WebMediaPlayerImpl::SetVolume(volume);
}

void WebMediaPlayerMSE::SetContentDecryptionModule(
    blink::WebContentDecryptionModule* cdm,
    blink::WebContentDecryptionModuleResult result) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  // call base-class implementation
  media::WebMediaPlayerImpl::SetContentDecryptionModule(cdm, result);
}

void WebMediaPlayerMSE::OnSuppressedMediaPlay(bool suppressed) {
  if (suppressed)
      Suspend();
  else
      Resume();
}

void WebMediaPlayerMSE::Suspend() {
  if (is_suspended_)
    return;

  status_on_suspended_ = (pipeline_controller_.GetPlaybackRate() == 0.0f)
                             ? PausedStatus
                             : PlayingStatus;

  if (status_on_suspended_ == PlayingStatus) {
    Pause();
  }

  if (media_platform_api_.get())
    media_platform_api_->Suspend();

  is_suspended_ = true;

  // TODO(neva): also need to set STORAGE_BLACK for VIDEO_HOLE ?
  if (HasVideo() && RenderTexture())
    video_frame_provider_->SetStorageType(media::VideoFrame::STORAGE_BLACK);
}

void WebMediaPlayerMSE::Resume() {
  if (!is_suspended_)
    return;
  is_suspended_ = false;

  media::MediaPlatformAPI::RestorePlaybackMode restore_playback_mode;

  restore_playback_mode = (status_on_suspended_ == PausedStatus)
                              ? media::MediaPlatformAPI::RESTORE_PAUSED
                              : media::MediaPlatformAPI::RESTORE_PLAYING;

  if (media_platform_api_.get()) {
    media_platform_api_->Resume(paused_time_, restore_playback_mode);
    UpdateVideoHoleBoundary(true);
  }

  if (status_on_suspended_ == PausedStatus) {
    Pause();
    status_on_suspended_ = UnknownStatus;
  } else {
    Play();
  }

  if (HasVideo() && RenderTexture())
    video_frame_provider_->SetStorageType(media::VideoFrame::STORAGE_OPAQUE);
}

void WebMediaPlayerMSE::OnMetadata(PipelineMetadata metadata) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  pipeline_metadata_ = metadata;

  UMA_HISTOGRAM_ENUMERATION("Media.VideoRotation",
                            metadata.video_decoder_config.video_rotation(),
                            VIDEO_ROTATION_MAX + 1);
  SetReadyState(WebMediaPlayer::kReadyStateHaveMetadata);

  if (HasVideo()) {
    if (pipeline_metadata_.video_decoder_config.video_rotation() ==
            VIDEO_ROTATION_90 ||
        pipeline_metadata_.video_decoder_config.video_rotation() ==
            VIDEO_ROTATION_270) {
      gfx::Size size = pipeline_metadata_.natural_size;
      pipeline_metadata_.natural_size = gfx::Size(size.height(), size.width());
    }

    // TODO(neva): |fullscreen_| has gone. need to check
    if (/*fullscreen_ &&*/ surface_manager_)
      surface_manager_->NaturalSizeChanged(pipeline_metadata_.natural_size);

    DCHECK(!video_layer_);

    video_frame_provider_->SetNaturalVideoSize(pipeline_metadata_.natural_size);
    video_frame_provider_->UpdateVideoFrame();

    video_layer_ = cc::VideoLayer::Create(
        video_frame_provider_.get(),
        pipeline_metadata_.video_decoder_config.video_rotation());
    video_layer_->SetContentsOpaque(opaque_);
    client_->SetCcLayer(video_layer_.get());
  }

  if (observer_)
    observer_->OnMetadataChanged(pipeline_metadata_);

  CreateWatchTimeReporter();
  UpdatePlayState();
}

// With updating the video hole position in every frame, Sometimes scrolling a
// page with a video element showes awkward delayed video-hole movement.
// Thus, this method uses a OneShotTimer to update the video position every
// 100ms, which is the previous implementation's position update period policy.
void WebMediaPlayerMSE::UpdateVideoHoleBoundary(bool forced) {
  // TODO: Need to remove throttleUpdateVideoHoleBoundary_ after improving
  // uMediaServer's performance of video hole position update.
  // Current uMeidaServer cannot update video-hole position smoothly at times.
  if (forced || !throttleUpdateVideoHoleBoundary_.IsRunning()) {
    if (!UpdateBoundaryRect()) {
      // UpdateBoundaryRect fails when video layer is not in current composition.
      if (HasVisibility()) {
        is_video_offscreen_ = true;
        SetVisibility(false);
      }
      return;
    }

    if (!ComputeVideoHoleDisplayRect(
            last_computed_rect_in_view_space_, NaturalSize(),
            additional_contents_scale_, client_->WebWidgetViewRect(),
            client_->ScreenRect(), source_rect_in_video_space_,
            visible_rect_in_screen_space_, is_fullscreen_)) {
        // visibile_rect_in_screen_space_ will be empty
        // when video position is out of the screen.
        if (visible_rect_in_screen_space_.IsEmpty() && HasVisibility()) {
          is_video_offscreen_ = true;
          SetVisibility(false);
        }
      return;
    }

    if (is_video_offscreen_) {
      SetVisibility(true);
      is_video_offscreen_ = false;
    }

    if (media_platform_api_) {
      LOG(INFO) << __func__ << " called SetDisplayWindow("
                << "out=[" << visible_rect_in_screen_space_.ToString() << "]"
                << ", in=[" << source_rect_in_video_space_.ToString() << "]"
                << ", is_fullscreen=" << is_fullscreen_
                << ")";
      media_platform_api_->SetDisplayWindow(visible_rect_in_screen_space_,
                                            source_rect_in_video_space_,
                                            is_fullscreen_);
    }

    if (!forced) {
      // The OneShotTimer, throttleUpdateVideoHoleBoundary_, is for correcting
      // the position of video-hole after scrolling.
      throttleUpdateVideoHoleBoundary_.Start(
          FROM_HERE,
          base::TimeDelta::FromMilliseconds(kMinVideoHoleUpdateInterval),
          base::Bind(&WebMediaPlayerMSE::UpdateVideoHoleBoundary,
                     base::Unretained(this), true));
    }
  }
}

// It returns true when it succeed to calcuate boundary rectangle.
// Returning false means videolayer is not created yet
// or layer doesn't transform on the screen space(no transform tree index).
// In other word, this means video is not shown on the screen if it returns false.
bool WebMediaPlayerMSE::UpdateBoundaryRect() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  // Check if video_layer_ is available.
  if (!video_layer_.get())
    return false;

  // Check if transform_tree_index of the layer is valid.
  if (video_layer_->transform_tree_index() == -1)
    return false;

  // Compute the geometry of video frame layer.
  gfx::RectF rect(gfx::SizeF(video_layer_->bounds()));
  video_layer_->ScreenSpaceTransform().TransformRect(&rect);

  // Store the changed geometry information when it is actually changed.
  last_computed_rect_in_view_space_ =
      gfx::Rect(rect.x(), rect.y(), rect.width(), rect.height());
  return true;
}

void WebMediaPlayerMSE::SetVisibility(bool visible) {
  media_platform_api_->SetVisibility(visible);
}

bool WebMediaPlayerMSE::HasVisibility() const {
  return media_platform_api_->Visibility();
}

void WebMediaPlayerMSE::OnDidCommitCompositorFrame() {
  if (!RenderTexture())
    UpdateVideoHoleBoundary(false);
}

scoped_refptr<VideoFrame> WebMediaPlayerMSE::GetCurrentFrameFromCompositor()
    const {
  TRACE_EVENT0("media", "WebMediaPlayerImpl::GetCurrentFrameFromCompositor");

  return video_frame_provider_->GetCurrentFrame();
}

void WebMediaPlayerMSE::SetRenderMode(blink::WebMediaPlayer::RenderMode mode) {
  if (render_mode_ == mode)
    return;

  render_mode_ = mode;
  if (RenderTexture()) {
    video_frame_provider_->SetStorageType(media::VideoFrame::STORAGE_OPAQUE);
#if defined(USE_VIDEO_TEXTURE)
    if (gfx::VideoTexture::IsSupported())
      media_platform_api_->SwitchToAutoLayout();
#endif
  } else {
#if defined(VIDEO_HOLE)
    video_frame_provider_->SetStorageType(media::VideoFrame::STORAGE_HOLE);
#endif
  }
}
}  // namespace media
