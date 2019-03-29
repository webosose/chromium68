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

#include "media/blink/neva/webmediaplayer_neva.h"

#include <algorithm>
#include <limits>
#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "cc/layers/video_layer.h"
#include "content/renderer/media/render_media_log.h"
#include "content/renderer/render_thread_impl.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "media/audio/null_audio_sink.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_content_type.h"
#include "media/base/media_log.h"
#include "media/base/media_switches.h"
#include "media/base/neva/media_constants.h"
#include "media/base/neva/video_util_neva.h"
#include "media/base/timestamp_constants.h"
#include "media/blink/neva/mediaplayerneva_factory.h"
#include "media/blink/webaudiosourceprovider_impl.h"
#include "media/blink/webmediaplayer_delegate.h"
#include "media/blink/webmediaplayer_params.h"
#include "media/blink/webmediaplayer_util.h"
#include "net/base/mime_util.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"
#include "third_party/blink/public/platform/web_fullscreen_video_status.h"
#include "third_party/blink/public/platform/web_media_player_source.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/public/web/web_view.h"

#include "content/public/renderer/render_frame.h"
#include "content/renderer/media/neva/stream_texture_factory.h"
#include "gpu/command_buffer/client/gles2_interface.h"

using gpu::gles2::GLES2Interface;

using blink::WebCanvas;
using blink::WebMediaPlayer;
using blink::WebRect;
using blink::WebSize;
using blink::WebString;
using blink::WebMediaPlayerClient;
using media::PipelineStatus;
using media::WebMediaPlayerParams;
using content::RenderFrame;

#define FUNC_LOG(x) VLOG(x) << __func__

namespace {

// Limits the range of playback rate.
const double kMinRate = -16.0;
const double kMaxRate = 16.0;

#if defined(VIDEO_HOLE)
static const int64_t kThrottleUpdateBoundaryDuration = 200;
#endif

const char* ReadyStateToString(WebMediaPlayer::ReadyState state) {
#define STRINGIFY_READY_STATUS_CASE(state) \
  case WebMediaPlayer::ReadyState::state:  \
    return #state

  switch (state) {
    STRINGIFY_READY_STATUS_CASE(kReadyStateHaveNothing);
    STRINGIFY_READY_STATUS_CASE(kReadyStateHaveMetadata);
    STRINGIFY_READY_STATUS_CASE(kReadyStateHaveCurrentData);
    STRINGIFY_READY_STATUS_CASE(kReadyStateHaveFutureData);
    STRINGIFY_READY_STATUS_CASE(kReadyStateHaveEnoughData);
  }
  return "null";
}

const char* NetworkStateToString(WebMediaPlayer::NetworkState state) {
#define STRINGIFY_NETWORK_STATUS_CASE(state) \
  case WebMediaPlayer::NetworkState::state:  \
    return #state

  switch (state) {
    STRINGIFY_NETWORK_STATUS_CASE(kNetworkStateEmpty);
    STRINGIFY_NETWORK_STATUS_CASE(kNetworkStateIdle);
    STRINGIFY_NETWORK_STATUS_CASE(kNetworkStateLoading);
    STRINGIFY_NETWORK_STATUS_CASE(kNetworkStateLoaded);
    STRINGIFY_NETWORK_STATUS_CASE(kNetworkStateFormatError);
    STRINGIFY_NETWORK_STATUS_CASE(kNetworkStateNetworkError);
    STRINGIFY_NETWORK_STATUS_CASE(kNetworkStateDecodeError);
  }
  return "null";
}

const char* MediaErrorToString(media::MediaPlayerNeva::MediaError error) {
#define STRINGIFY_MEDIA_ERROR_CASE(error)         \
  case media::MediaPlayerNeva::MediaError::error: \
    return #error

  switch (error) {
    STRINGIFY_MEDIA_ERROR_CASE(MEDIA_ERROR_NONE);
    STRINGIFY_MEDIA_ERROR_CASE(MEDIA_ERROR_FORMAT);
    STRINGIFY_MEDIA_ERROR_CASE(MEDIA_ERROR_DECODE);
    STRINGIFY_MEDIA_ERROR_CASE(MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK);
    STRINGIFY_MEDIA_ERROR_CASE(MEDIA_ERROR_INVALID_CODE);
  }
  return "null";
}

bool IsBackgroundedSuspendEnabled() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableMediaSuspend);
}

class SyncTokenClientImpl : public media::VideoFrame::SyncTokenClient {
 public:
  explicit SyncTokenClientImpl(gpu::gles2::GLES2Interface* gl) : gl_(gl) {}
  ~SyncTokenClientImpl() override {}
  void GenerateSyncToken(gpu::SyncToken* sync_token) override {
    gl_->GenSyncTokenCHROMIUM(sync_token->GetData());
  }
  void WaitSyncToken(const gpu::SyncToken& sync_token) override {
    gl_->WaitSyncTokenCHROMIUM(sync_token.GetConstData());
  }

 private:
  gpu::gles2::GLES2Interface* gl_;
};

}  // namespace

namespace media {

blink::WebMediaPlayer* WebMediaPlayerNeva::Create(
    blink::WebLocalFrame* frame,
    blink::WebMediaPlayerClient* client,
    WebMediaPlayerDelegate* delegate,
    const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
    std::unique_ptr<WebMediaPlayerParams> params,
    std::unique_ptr<WebMediaPlayerParamsNeva> params_neva) {
  blink::WebMediaPlayer::LoadType load_type = client->LoadType();
  if (load_type == blink::WebMediaPlayer::kLoadTypeURL &&
      MediaPlayerNevaFactory::CanSupportMediaType(
          client->ContentMIMEType().Latin1()))
    return new WebMediaPlayerNeva(frame, client, delegate,
                                  stream_texture_factory_create_cb,
                                  std::move(params), std::move(params_neva));
  return nullptr;
}

bool WebMediaPlayerNeva::CanSupportMediaType(const std::string& mime) {
  return MediaPlayerNevaFactory::CanSupportMediaType(mime);
}

WebMediaPlayerNeva::WebMediaPlayerNeva(
    blink::WebLocalFrame* frame,
    blink::WebMediaPlayerClient* client,
    WebMediaPlayerDelegate* delegate,
    const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
    std::unique_ptr<WebMediaPlayerParams> params,
    std::unique_ptr<WebMediaPlayerParamsNeva> params_neva)
    : frame_(frame),
      main_task_runner_(
          frame->GetTaskRunner(blink::TaskType::kMediaElementEvent)),
      client_(client),
      delegate_(delegate),
      delegate_id_(0),
      defer_load_cb_(params->defer_load_cb()),
      buffered_(static_cast<size_t>(1)),
      pending_seek_(false),
      seeking_(false),
      did_loading_progress_(false),
      network_state_(WebMediaPlayer::kNetworkStateEmpty),
      ready_state_(WebMediaPlayer::kReadyStateHaveNothing),
      is_playing_(false),
      has_size_info_(false),
      video_frame_provider_client_(NULL),
      last_computed_rect_changed_since_updated_(false),
      media_log_(params->take_media_log()),
      interpolator_(&default_tick_clock_),
      playback_completed_(false),
      is_suspended_(client->IsSuppressedMediaPlay()),
      status_on_suspended_(UnknownStatus),
      // Threaded compositing isn't enabled universally yet.
      compositor_task_runner_(params->compositor_task_runner()
                                  ? params->compositor_task_runner()
                                  : base::ThreadTaskRunnerHandle::Get()),
      render_mode_(blink::WebMediaPlayer::RenderModeNone),
      content_position_offset_(0.0f),
      additional_contents_scale_(params_neva->additional_contents_scale()),
      is_fullscreen_(false),
      is_fullscreen_mode_(false),
      active_video_region_changed_(false),
      is_video_offscreen_(false),
      app_id_(params_neva->application_id().Utf8().data()),
      is_loading_(false) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (delegate_)
    delegate_id_ = delegate_->AddObserver(this);

  media_log_->AddEvent(
      media_log_->CreateEvent(media::MediaLogEvent::WEBMEDIAPLAYER_CREATED));

  // We want to be notified of |main_loop_| destruction.
  base::MessageLoop::current()->AddDestructionObserver(this);

  // Use the null sink if no sink was provided.
  audio_source_provider_ = new WebAudioSourceProviderImpl(
      params->audio_renderer_sink(), media_log_.get());

  std::string mime_type = client_->ContentMIMEType().Latin1();
  player_api_.reset(MediaPlayerNevaFactory::CreateMediaPlayerNeva(
      this, mime_type, main_task_runner_, app_id_));

  video_frame_provider_ = std::make_unique<VideoFrameProviderImpl>(
      stream_texture_factory_create_cb, compositor_task_runner_);
  video_frame_provider_->SetWebLocalFrame(frame);
  video_frame_provider_->SetWebMediaPlayerClient(client);
  SetRenderMode(GetClient()->RenderMode());
  base::Optional<bool> is_audio_disabled = GetClient()->IsAudioDisabled();
  if (is_audio_disabled.has_value()) {
    SetDisableAudio(*is_audio_disabled);
  }

  bool require_media_resource = player_api_->RequireMediaResource() &&
                                !params_neva->use_unlimited_media_policy();
  delegate_->DidMediaCreated(delegate_id_, require_media_resource);
}

WebMediaPlayerNeva::~WebMediaPlayerNeva() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  GetClient()->SetCcLayer(nullptr);

  if (video_layer_.get()) {
    video_layer_->StopUsingProvider();
  }
  compositor_task_runner_->DeleteSoon(FROM_HERE,
                                      std::move(video_frame_provider_));

  if (throttle_update_video_hole_boundary_.IsRunning())
    throttle_update_video_hole_boundary_.Stop();

  media_log_->AddEvent(
      media_log_->CreateEvent(media::MediaLogEvent::WEBMEDIAPLAYER_DESTROYED));

  if (delegate_) {
    delegate_->PlayerGone(delegate_id_);
    delegate_->RemoveObserver(delegate_id_);
  }

  // Remove destruction observer if we're being destroyed but the main thread is
  // still running.
  if (base::MessageLoop::current())
    base::MessageLoop::current()->RemoveDestructionObserver(this);
}

void WebMediaPlayerNeva::Load(LoadType load_type,
                              const blink::WebMediaPlayerSource& src,
                              CORSMode cors_mode) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  LOG(INFO) << __func__;

  DCHECK(src.IsURL());

  is_loading_ = true;

  // If preloading is expected, do load without permit from MediaStateManager.
  if (player_api_->IsPreloadable(
          GetClient()->ContentMediaOption().Utf8().data())) {
    DoLoad(load_type, src.GetAsURL(), cors_mode);
    return;
  }

  pending_load_type_ = load_type;
  pending_source_ = blink::WebMediaPlayerSource(src.GetAsURL());
  pending_cors_mode_ = cors_mode;

  delegate_->DidMediaActivationNeeded(delegate_id_);
}

void WebMediaPlayerNeva::UpdatePlayingState(bool is_playing) {
  FUNC_LOG(1);
  if (is_playing == is_playing_)
    return;

  is_playing_ = is_playing;

  if (is_playing)
    interpolator_.StartInterpolating();
  else
    interpolator_.StopInterpolating();

  if (delegate_) {
    if (is_playing) {
      // We must specify either video or audio to the delegate, but neither may
      // be known at this point -- there are no video only containers, so only
      // send audio if we know for sure its audio.  The browser side player will
      // fill in the correct value later for media sessions.
      delegate_->DidPlay(delegate_id_, HasVideo(), !HasVideo(),
                         media::MediaContentType::Persistent /*, duration_*/);
    } else {
      // Even if OnPlaybackComplete() has not been called yet, Blink may have
      // already fired the ended event based on current time relative to
      // duration -- so we need to check both possibilities here.
      delegate_->DidPause(delegate_id_);
    }
  }
}

void WebMediaPlayerNeva::DoLoad(LoadType load_type,
                                const blink::WebURL& url,
                                CORSMode cors_mode) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  // We should use MediaInfoLoader for all URLs but because of missing
  // scheme handlers in WAM we use it only for file scheme for now.
  // By using MediaInfoLoader url gets passed to network delegate which
  // does proper whitelist filtering for local file access.
  GURL mediaUrl(url);
  if (mediaUrl.SchemeIsFile() || mediaUrl.SchemeIsFileSystem()) {
    info_loader_.reset(new MediaInfoLoader(
        mediaUrl,
        base::Bind(&WebMediaPlayerNeva::DidLoadMediaInfo, AsWeakPtr())));
    info_loader_->Start(frame_);

    UpdateNetworkState(WebMediaPlayer::kNetworkStateLoading);
    UpdateReadyState(WebMediaPlayer::kReadyStateHaveNothing);
  } else {
    UpdateNetworkState(WebMediaPlayer::kNetworkStateLoading);
    UpdateReadyState(WebMediaPlayer::kReadyStateHaveNothing);
    DidLoadMediaInfo(true, mediaUrl);
  }
}

void WebMediaPlayerNeva::DidLoadMediaInfo(bool ok, const GURL& url) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  if (!ok) {
    info_loader_.reset();
    UpdateNetworkState(WebMediaPlayer::kNetworkStateNetworkError);
    return;
  }

  media_log_->AddEvent(media_log_->CreateLoadEvent(url.spec()));
  url_ = url;

  LoadMedia();
}

void WebMediaPlayerNeva::LoadMedia() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);

  std::string mime_type(GetClient()->ContentMIMEType().Utf8().data());
  std::string payload(GetClient()->ContentMediaOption().Utf8().data());
  if (mime_type == std::string("service/webos-camera"))
    payload = std::string(GetClient()->ContentCustomOption().Utf8().data());

  player_api_->Initialize(
      GetClient()->IsVideo(), CurrentTime(), app_id_, url_.spec(), mime_type,
      std::string(GetClient()->Referrer().Utf8().data()),
      std::string(GetClient()->UserAgent().Utf8().data()),
      std::string(GetClient()->Cookies().Utf8().data()), payload);
}

void WebMediaPlayerNeva::OnActiveRegionChanged(
    const blink::WebRect& active_region) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  LOG(INFO) << __func__ << " (" << gfx::Rect(active_region).ToString() << ")";
  video_frame_provider_->ActiveRegionChanged(active_region);
  if (!NaturalSize().IsEmpty())
    video_frame_provider_->UpdateVideoFrame();
}

void WebMediaPlayerNeva::Play() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  LOG(INFO) << __func__;
  if (!has_activation_permit_) {
    LOG(INFO) << "block to play on suspended";
    status_on_suspended_ = PlayingStatus;
    if (!client_->IsSuppressedMediaPlay())
      delegate_->DidMediaActivationNeeded(delegate_id_);
    return;
  }

  UpdatePlayingState(true);
  player_api_->Start();

  media_log_->AddEvent(media_log_->CreateEvent(media::MediaLogEvent::PLAY));

  if (delegate_) {
    delegate_->DidPlay(
        delegate_id_, HasVideo(), HasAudio(),
        media::MediaContentType::Persistent);  // TODO(wanchang): check
                                               // MediaContentType
  }
}

void WebMediaPlayerNeva::Pause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  LOG(INFO) << __func__;

  UpdatePlayingState(false);
  player_api_->Pause();

  paused_time_ = base::TimeDelta::FromSecondsD(CurrentTime());

  media_log_->AddEvent(media_log_->CreateEvent(media::MediaLogEvent::PAUSE));

  if (delegate_) {
    delegate_->DidPause(delegate_id_ /*, false*/);  // TODO: capture EOS
  }
}

void WebMediaPlayerNeva::EnteredFullscreen() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  if (!is_fullscreen_mode_) {
    is_fullscreen_mode_ = true;
    UpdateVideoHoleBoundary(true);
  }
}

void WebMediaPlayerNeva::ExitedFullscreen() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  if (is_fullscreen_mode_) {
    is_fullscreen_mode_ = false;
    UpdateVideoHoleBoundary(true);
  }
}

void WebMediaPlayerNeva::BecameDominantVisibleContent(bool is_dominant) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1) << "is_dominant=" << is_dominant;
}

void WebMediaPlayerNeva::SetIsEffectivelyFullscreen(
    blink::WebFullscreenVideoStatus status) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1) << "fs status=" << (int)status;
}

void WebMediaPlayerNeva::Seek(double seconds) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);

  playback_completed_ = false;

  // TODO(neva): We may need ConvertSecondsToTimestamp here
  // See RP's webmediaplayer_util.h.
  base::TimeDelta new_seek_time = base::TimeDelta::FromSecondsD(seconds);

  if (seeking_) {
    if (new_seek_time == seek_time_) {
      // Suppress all redundant seeks if unrestricted by media source
      // demuxer API.
      pending_seek_ = false;
      return;
    }

    pending_seek_ = true;
    pending_seek_time_ = new_seek_time;

    return;
  }

  seeking_ = true;
  seek_time_ = new_seek_time;

  // Kick off the asynchronous seek!
  player_api_->Seek(seek_time_);
  media_log_->AddEvent(media_log_->CreateSeekEvent(seconds));
}

void WebMediaPlayerNeva::SetRate(double rate) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);

  // Limit rates to reasonable values by clamping.
  rate = std::max(kMinRate, std::min(rate, kMaxRate));

  if (!has_activation_permit_) {
    LOG(INFO) << "block to setRate on suspended";
    if (!client_->IsSuppressedMediaPlay())
      delegate_->DidMediaActivationNeeded(delegate_id_);
    return;
  }

  player_api_->SetRate(rate);
  is_negative_playback_rate_ = rate < 0.0f;
}

void WebMediaPlayerNeva::SetVolume(double volume) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);

  volume_ = volume;
  player_api_->SetVolume(volume_);
}

void WebMediaPlayerNeva::SetPreload(Preload preload) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  switch (preload) {
    case WebMediaPlayer::kPreloadNone:
      player_api_->SetPreload(MediaPlayerNeva::PreloadNone);
      break;
    case WebMediaPlayer::kPreloadMetaData:
      player_api_->SetPreload(MediaPlayerNeva::PreloadMetaData);
      break;
    case WebMediaPlayer::kPreloadAuto:
      player_api_->SetPreload(MediaPlayerNeva::PreloadAuto);
      break;
    default:
      break;
  }
}

bool WebMediaPlayerNeva::HasVideo() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(2);
  return player_api_->HasVideo();
}

bool WebMediaPlayerNeva::HasAudio() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(2);
  return player_api_->HasAudio();
}

bool WebMediaPlayerNeva::Paused() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  return !is_playing_;
}

bool WebMediaPlayerNeva::Seeking() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return seeking_;
}

double WebMediaPlayerNeva::Duration() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (ready_state_ == WebMediaPlayer::kReadyStateHaveNothing)
    return std::numeric_limits<double>::quiet_NaN();

  return duration_.InSecondsF();
}

double WebMediaPlayerNeva::CurrentTime() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // If the player is processing a seek, return the seek time.
  // Blink may still query us if updatePlaybackState() occurs while seeking.
  if (Seeking()) {
    return pending_seek_ ? pending_seek_time_.InSecondsF()
                         : seek_time_.InSecondsF();
  }

  double current_time =
      std::min((const_cast<media::TimeDeltaInterpolator*>(&interpolator_))
                   ->GetInterpolatedTime(),
               duration_)
          .InSecondsF();

  // The time of interpolator updated from UMediaClient could be a little bigger
  // than the correct current time, this makes |current_time| a negative number
  // after the plaback time reaches at 0:00 by rewinding.
  // this conditional statement sets current_time's lower bound which is 00:00
  if (current_time < 0)
    current_time = 0;

  return current_time;
}

WebSize WebMediaPlayerNeva::NaturalSize() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return natural_size_;
}

WebSize WebMediaPlayerNeva::VisibleRect() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // FIXME: Need to check visible rect: really it is natural size.
  return natural_size_;
}

WebMediaPlayer::NetworkState WebMediaPlayerNeva::GetNetworkState() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return network_state_;
}

WebMediaPlayer::ReadyState WebMediaPlayerNeva::GetReadyState() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return ready_state_;
}

blink::WebString WebMediaPlayerNeva::GetErrorMessage() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return blink::WebString();
}

blink::WebTimeRanges WebMediaPlayerNeva::Buffered() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return buffered_;
}

blink::WebTimeRanges WebMediaPlayerNeva::Seekable() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (ready_state_ < WebMediaPlayer::kReadyStateHaveMetadata)
    return blink::WebTimeRanges();

  // TODO(dalecurtis): Technically this allows seeking on media which return an
  // infinite duration.  While not expected, disabling this breaks semi-live
  // players, http://crbug.com/427412.
  const blink::WebTimeRange seekable_range(0.0, Duration());
  return blink::WebTimeRanges(&seekable_range, 1);
}

bool WebMediaPlayerNeva::DidLoadingProgress() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  bool ret = did_loading_progress_;
  did_loading_progress_ = false;
  return ret;
}

void WebMediaPlayerNeva::Paint(blink::WebCanvas* canvas,
                               const blink::WebRect& rect,
                               cc::PaintFlags& flags,
                               int already_uploaded_id,
                               VideoFrameUploadMetadata* out_metadata) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // TODO(wanchang): check android impl
  return;
}

// TODO(neva): need to check why |DidGetOpaqueResponseFromServiceWorker| is
// added.
bool WebMediaPlayerNeva::DidGetOpaqueResponseFromServiceWorker() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return false;
}

bool WebMediaPlayerNeva::HasSingleSecurityOrigin() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // TODO(wanchang): check android impl
  return true;
}

bool WebMediaPlayerNeva::DidPassCORSAccessCheck() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // TODO(wanchang): check android impl
  return false;
}

double WebMediaPlayerNeva::MediaTimeForTimeValue(double timeValue) const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return base::TimeDelta::FromSecondsD(timeValue).InSecondsF();
}

unsigned WebMediaPlayerNeva::DecodedFrameCount() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // TODO(wanchang): check android impl
  return 0;
}

unsigned WebMediaPlayerNeva::DroppedFrameCount() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // TODO(wanchang): check android impl
  return 0;
}

size_t WebMediaPlayerNeva::AudioDecodedByteCount() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // TODO(wanchang): check android impl
  return 0;
}

size_t WebMediaPlayerNeva::VideoDecodedByteCount() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // TODO(wanchang): check android impl
  return 0;
}

void WebMediaPlayerNeva::OnMediaMetadataChanged(base::TimeDelta duration,
                                                int width,
                                                int height,
                                                bool success) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);

  bool need_to_signal_duration_changed = false;

  // For HLS streams, the reported duration may be zero for infinite streams.
  // See http://crbug.com/501213.
  if (duration.is_zero() && IsHLSStream())
    duration = media::kInfiniteDuration;

  // Update duration, if necessary, prior to ready state updates that may
  // cause duration() query.
  if (duration_ != duration) {
    duration_ = duration;
    // Client readyState transition from HAVE_NOTHING to HAVE_METADATA
    // already triggers a durationchanged event. If this is a different
    // transition, remember to signal durationchanged.
    if (ready_state_ > WebMediaPlayer::kReadyStateHaveNothing) {
      need_to_signal_duration_changed = true;
    }
  }

  if (ready_state_ < WebMediaPlayer::kReadyStateHaveMetadata)
    UpdateReadyState(WebMediaPlayer::kReadyStateHaveMetadata);

  // TODO(wolenetz): Should we just abort early and set network state to an
  // error if success == false? See http://crbug.com/248399
  if (success) {
    OnVideoSizeChanged(width, height);
  }

  if (need_to_signal_duration_changed)
    client_->DurationChanged();
}

void WebMediaPlayerNeva::OnLoadComplete() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  is_loading_ = false;
  if (ready_state_ < WebMediaPlayer::kReadyStateHaveEnoughData)
    UpdateReadyState(WebMediaPlayer::kReadyStateHaveEnoughData);
  delegate_->DidMediaActivated(delegate_id_);
}

void WebMediaPlayerNeva::OnPlaybackComplete() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // When playback is about to finish, android media player often stops
  // at a time which is smaller than the duration. This makes webkit never
  // know that the playback has finished. To solve this, we set the
  // current time to media duration when OnPlaybackComplete() get called.
  // But in case of negative playback, we set the current time to zero.
  base::TimeDelta bound =
      is_negative_playback_rate_ ? base::TimeDelta() : duration_;
  interpolator_.SetBounds(
      bound, bound,
      base::TimeTicks::Now());  // TODO(wanchang): fix 3rd argument
  client_->TimeChanged();

  // If the loop attribute is set, timeChanged() will update the current time
  // to 0. It will perform a seek to 0. Issue a command to the player to start
  // playing after seek completes.
  if (is_playing_ && seeking_ && seek_time_.is_zero())
    player_api_->Start();
  else
    playback_completed_ = true;
}

void WebMediaPlayerNeva::OnBufferingUpdate(int percentage) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  buffered_[0].end = Duration() * percentage / 100;
  did_loading_progress_ = true;
  did_loading_progress_ = true;

  if (percentage == 100 && network_state_ < WebMediaPlayer::kNetworkStateLoaded)
    UpdateNetworkState(WebMediaPlayer::kNetworkStateLoaded);
}

void WebMediaPlayerNeva::OnSeekComplete(const base::TimeDelta& current_time) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  seeking_ = false;
  if (pending_seek_) {
    pending_seek_ = false;
    Seek(pending_seek_time_.InSecondsF());
    return;
  }
  interpolator_.SetBounds(current_time, current_time, base::TimeTicks::Now());

  UpdateReadyState(WebMediaPlayer::kReadyStateHaveEnoughData);

  client_->TimeChanged();
}

void WebMediaPlayerNeva::OnMediaError(int error_type) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  LOG(ERROR) << __func__ << "("
             << MediaErrorToString((MediaPlayerNeva::MediaError)error_type)
             << ")";

  if (is_loading_) {
    is_loading_ = false;
    delegate_->DidMediaActivated(delegate_id_);
  }

  switch (error_type) {
    case MediaPlayerNeva::MEDIA_ERROR_FORMAT:
      UpdateNetworkState(WebMediaPlayer::kNetworkStateFormatError);
      break;
    case MediaPlayerNeva::MEDIA_ERROR_DECODE:
      UpdateNetworkState(WebMediaPlayer::kNetworkStateDecodeError);
      break;
    case MediaPlayerNeva::MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK:
      UpdateNetworkState(WebMediaPlayer::kNetworkStateFormatError);
      break;
    case MediaPlayerNeva::MEDIA_ERROR_INVALID_CODE:
      break;
  }
  client_->Repaint();
}

void WebMediaPlayerNeva::OnVideoSizeChanged(int width, int height) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);

  // Ignore OnVideoSizeChanged before kReadyStateHaveMetadata.
  // OnVideoSizeChanged will be called again from OnMediaMetadataChanged
  if (ready_state_ < WebMediaPlayer::kReadyStateHaveMetadata)
    return;

  // For HLS streams, a bogus empty size may be reported at first, followed by
  // the actual size only once playback begins. See http://crbug.com/509972.
  if (!has_size_info_ && width == 0 && height == 0 && IsHLSStream())
    return;

  has_size_info_ = true;
  if (natural_size_.width == width && natural_size_.height == height)
    return;

  natural_size_.width = width;
  natural_size_.height = height;

  client_->SizeChanged();

  // set video size first then update videoframe since videoframe
  // needs video size.
  video_frame_provider_->SetNaturalVideoSize(NaturalSize());
  video_frame_provider_->UpdateVideoFrame();

  // Lazily allocate compositing layer.
  if (!video_layer_) {
    video_layer_ = cc::VideoLayer::Create(video_frame_provider_.get(),
                                          media::VIDEO_ROTATION_0);
    client_->SetCcLayer(video_layer_.get());

    // If we're paused after we receive metadata for the first time, tell the
    // delegate we can now be safely suspended due to inactivity if a subsequent
    // play event does not occur.
    if (Paused() && delegate_)
      delegate_->DidPause(
          delegate_id_ /*, false*/);  // TODO(wanchang): check 2nd argument
  }

#if defined(VIDEO_HOLE)
  if (!RenderTexture())
    UpdateVideoHoleBoundary(true);
#endif
}

void WebMediaPlayerNeva::OnAudioTracksUpdated(
    const std::vector<struct MediaTrackInfo>& audio_track_info) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  for (auto& audio_track : audio_track_info) {
    // Check current id is already added or not.
    auto it = std::find_if(audio_track_ids_.begin(), audio_track_ids_.end(),
                           [&audio_track](const MediaTrackId& id) {
                             return audio_track.id == id.second;
                           });
    if (it != audio_track_ids_.end())
      continue;

    // TODO(neva): Use kind info. And as per comment in WebMediaPlayerImpl,
    // only the first audio track is enabled by default to match blink logic.
    WebMediaPlayer::TrackId track_id = GetClient()->AddAudioTrack(
        blink::WebString::FromUTF8(audio_track.id),
        blink::WebMediaPlayerClient::kAudioTrackKindMain,
        blink::WebString::FromUTF8("Audio Track"),
        blink::WebString::FromUTF8(audio_track.language), false);
    if (!track_id.IsNull() && !track_id.IsEmpty())
      audio_track_ids_.push_back(MediaTrackId(track_id, audio_track.id));
  }

  // TODO(neva): Should we remove unavailable audio track?
}

void WebMediaPlayerNeva::OnTimeUpdate(base::TimeDelta current_timestamp,
                                      base::TimeTicks current_time_ticks) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (Seeking())
    return;

  // Compensate the current_timestamp with the IPC latency.
  base::TimeDelta lower_bound =
      base::TimeTicks::Now() - current_time_ticks + current_timestamp;
  base::TimeDelta upper_bound = lower_bound;
  // We should get another time update in about |kTimeUpdateInterval|
  // milliseconds.
  if (is_playing_) {
    upper_bound += base::TimeDelta::FromMilliseconds(kTimeUpdateInterval);
  }
  // if the lower_bound is smaller than the current time, just use the current
  // time so that the timer is always progressing.
  lower_bound =
      std::max(lower_bound, base::TimeDelta::FromSecondsD(CurrentTime()));
  if (lower_bound > upper_bound)
    upper_bound = lower_bound;
  interpolator_.SetBounds(
      lower_bound, upper_bound,
      current_time_ticks);  // TODO(wanchang): check 3rd argument
}

void WebMediaPlayerNeva::OnMediaPlayerPlay() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  UpdatePlayingState(true);
  client_->PlaybackStateChanged();
}

void WebMediaPlayerNeva::OnMediaPlayerPause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  UpdatePlayingState(false);
  client_->PlaybackStateChanged();
}

void WebMediaPlayerNeva::UpdateNetworkState(
    WebMediaPlayer::NetworkState state) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  VLOG(1) << __func__ << "(" << NetworkStateToString(state) << ")";
  if (ready_state_ == WebMediaPlayer::kReadyStateHaveNothing &&
      (state == WebMediaPlayer::kNetworkStateNetworkError ||
       state == WebMediaPlayer::kNetworkStateDecodeError)) {
    // Any error that occurs before reaching ReadyStateHaveMetadata should
    // be considered a format error.
    network_state_ = WebMediaPlayer::kNetworkStateFormatError;
  } else {
    network_state_ = state;
  }
  // Always notify to ensure client has the latest value.
  GetClient()->NetworkStateChanged();
}

void WebMediaPlayerNeva::UpdateReadyState(WebMediaPlayer::ReadyState state) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  VLOG(1) << __func__ << "(" << ReadyStateToString(state) << ")";

  if (state == WebMediaPlayer::kReadyStateHaveEnoughData &&
      url_.SchemeIs("file") &&
      network_state_ == WebMediaPlayer::kNetworkStateLoading)
    UpdateNetworkState(WebMediaPlayer::kNetworkStateLoaded);

  ready_state_ = state;
  // Always notify to ensure client has the latest value.
  GetClient()->ReadyStateChanged();
}

// Do a GPU-GPU texture copy of the current video frame to |texture|,
// reallocating |texture| at the appropriate size with given internal
// format, format, and type if necessary. If the copy is impossible
// or fails, it returns false.

// TODO(wanchang): |target| |level| are added. need to check why they are added.
bool WebMediaPlayerNeva::CopyVideoTextureToPlatformTexture(
    gpu::gles2::GLES2Interface* web_graphics_context,
    unsigned int target,
    unsigned int texture,
    unsigned internal_format,
    unsigned format,
    unsigned type,
    int level,
    bool premultiply_alpha,
    bool flip_y,
    int already_uploaded_id,
    VideoFrameUploadMetadata* out_metadata) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (!RenderTexture())
    return false;
  scoped_refptr<media::VideoFrame> video_frame;
  {
    base::AutoLock auto_lock(video_frame_provider_->GetLock());
    video_frame = video_frame_provider_->GetCurrentFrame();
  }

  if (!video_frame.get() || !video_frame->HasTextures()) {
    return false;
  }

  const gpu::MailboxHolder& mailbox_holder = video_frame->mailbox_holder(0);
  if (mailbox_holder.texture_target != GL_TEXTURE_2D) {
    return false;
  }

// Since this method changes which texture is bound to the TEXTURE_2D target,
// ideally it would restore the currently-bound texture before returning.
// The cost of getIntegerv is sufficiently high, however, that we want to
// avoid it in user builds. As a result assume (below) that |texture| is
// bound when this method is called, and only verify this fact when
// DCHECK_IS_ON.
#if DCHECK_IS_ON()
  GLint bound_texture = 0;
  web_graphics_context->GetIntegerv(GL_TEXTURE_BINDING_2D, &bound_texture);
  DCHECK_EQ(static_cast<GLuint>(bound_texture), texture);
#endif

  web_graphics_context->WaitSyncTokenCHROMIUM(
      mailbox_holder.sync_token.GetConstData());

  uint32_t src_texture = web_graphics_context->CreateAndConsumeTextureCHROMIUM(
      mailbox_holder.mailbox.name);

  // Application itself needs to take care of setting the right flip_y
  // value down to get the expected result.
  // flip_y==true means to reverse the video orientation while
  // flip_y==false means to keep the intrinsic orientation.
  web_graphics_context->CopyTextureCHROMIUM(
      src_texture, 0, mailbox_holder.texture_target, texture, level,
      internal_format, type, flip_y, premultiply_alpha, false);

  web_graphics_context->DeleteTextures(1, &src_texture);

  // The flush() operation is not necessary here. It is kept since the
  // performance will be better when it is added than not.
  web_graphics_context->Flush();

  SyncTokenClientImpl client(web_graphics_context);
  video_frame->UpdateReleaseSyncToken(&client);
  return true;
}

blink::WebAudioSourceProvider* WebMediaPlayerNeva::GetAudioSourceProvider() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return audio_source_provider_.get();
}

void WebMediaPlayerNeva::WillDestroyCurrentMessageLoop() {}

void WebMediaPlayerNeva::Repaint() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  GetClient()->Repaint();
}

blink::WebRect WebMediaPlayerNeva::ScaleWebRect(const blink::WebRect& rect,
                                                blink::WebFloatPoint scale) {
  blink::WebRect scaledRect;

  scaledRect.x = rect.x * scale.x;
  scaledRect.y = rect.y * scale.y;
  scaledRect.width = rect.width * scale.x;
  scaledRect.height = rect.height * scale.y;

  return scaledRect;
}

#if defined(VIDEO_HOLE)
void WebMediaPlayerNeva::SetDisplayWindow() {
  LOG(INFO) << __func__ << " called SetDisplayWindow("
            << "out=[" << visible_rect_in_screen_space_.ToString() << "]"
            << ", in=[" << source_rect_in_video_space_.ToString() << "]"
            << ", is_fullscreen=" << is_fullscreen_ << ")";
  player_api_->SetDisplayWindow(visible_rect_in_screen_space_,
                                source_rect_in_video_space_, is_fullscreen_,
                                true);
  last_computed_rect_changed_since_updated_ = false;
}

// With updating the video hole position in every frame, Sometimes scrolling a
// page with a video element showes awkward delayed video-hole movement.
// Thus, this method uses a OneShotTimer to update the video position every
// 100ms, which is the previous implementation's position update period policy.
void WebMediaPlayerNeva::UpdateVideoHoleBoundary(bool forced) {
  // TODO: Need to remove throttle_update_video_hole_boundary_ after improving
  // uMediaServer's performance of video hole position update.
  // Current uMeidaServer cannot update video-hole position smoothly at times.
  if (forced || !throttle_update_video_hole_boundary_.IsRunning()) {
    if (!ComputeVideoHoleDisplayRect(
            last_computed_rect_in_view_space_, NaturalSize(),
            additional_contents_scale_, client_->WebWidgetViewRect(),
            client_->ScreenRect(), is_fullscreen_mode_,
            source_rect_in_video_space_, visible_rect_in_screen_space_,
            is_fullscreen_)) {
      // visibile_rect_in_screen_space_ will be empty
      // when video position is out of the screen.
      if (visible_rect_in_screen_space_.IsEmpty() && HasVisibility()) {
        is_video_offscreen_ = true;
        SetVisibility(false);
        return;
      }
      // If forced update is used or video was offscreen, it needs to update
      // even though video position is not changed.
      // Also even though video position is not changed if
      // last_computed_rect_changed_since_updated_ is true, then we need to
      // update video position since source_rect_in_video_space_ might be
      // changed. Possibly source_rect_in_screen_space_ might be changed
      // without last_computed_rect_changed_since_updated_ is true when
      // natural_size is changed (i.e DASH) in this case we ignore it because
      // framework will handle it
      if (!forced && !is_video_offscreen_ &&
          !last_computed_rect_changed_since_updated_)
        return;
    }

    if (is_video_offscreen_) {
      SetVisibility(true);
      is_video_offscreen_ = false;
    }

    if (ready_state_ < WebMediaPlayer::kReadyStateHaveMetadata) {
      LOG(INFO) << __func__
                << " Aborting setDisplayWindow, ready_state_:" << ready_state_
                << " visible_rect_in_screen_space_: "
                << visible_rect_in_screen_space_.ToString();
      visible_rect_in_screen_space_ = WebRect();
    } else if (player_api_) {
      SetDisplayWindow();
      if (!forced)
        // The OneShotTimer, throttle_update_video_hole_boundary_, is for
        // correcting the position of video-hole after scrolling.
        throttle_update_video_hole_boundary_.Start(
            FROM_HERE,
            base::TimeDelta::FromMilliseconds(kThrottleUpdateBoundaryDuration),
            base::Bind(&WebMediaPlayerNeva::UpdateVideoHoleBoundary,
                       base::Unretained(this), true));
    }
  }
}

// It returns true when it succeed to calcuate boundary rectangle.
// Returning false means videolayer is not created yet
// or layer doesn't transform on the screen space(no transform tree index).
// In other word, this means video is not shown on the screen if it returns false.
// Note: This api should be called only from |OnDidCommitCompositorFrame|
bool WebMediaPlayerNeva::UpdateBoundaryRectangle() {
  // Check if video_layer_ is available.
  if (!video_layer_.get())
    return false;

  // Check if transform_tree_index of the layer is valid.
  if (video_layer_->transform_tree_index() == -1)
    return false;

  // Compute the geometry of video frame layer.
  gfx::RectF rect(gfx::SizeF(video_layer_->bounds()));
  gfx::Transform transform = video_layer_->ScreenSpaceTransform();
  transform.TransformRect(&rect);

  // Check if video layer position is changed.
  gfx::Rect video_rect(rect.x(), rect.y(), rect.width(), rect.height());
  if (!last_computed_rect_changed_since_updated_ &&
      video_rect != last_computed_rect_in_view_space_)
    last_computed_rect_changed_since_updated_ = true;

  // Store the changed geometry information when it is actually changed.
  last_computed_rect_in_view_space_ = video_rect;
  return true;
}
#endif

blink::WebMediaPlayerClient* WebMediaPlayerNeva::GetClient() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DCHECK(client_);
  return client_;
}

void WebMediaPlayerNeva::OnSuspend() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (is_suspended_) {
    delegate_->DidMediaSuspended(delegate_id_);
    return;
  }

  is_suspended_ = true;
  has_activation_permit_ = false;
  status_on_suspended_ = Paused() ? PausedStatus : PlayingStatus;
  if (status_on_suspended_ == PlayingStatus) {
    Pause();
    GetClient()->PlaybackStateChanged();
  }
  if (HasVideo()) {
    video_frame_provider_->SetStorageType(media::VideoFrame::STORAGE_BLACK);
  }
  SuspendReason reason = client_->IsSuppressedMediaPlay()
                             ? SuspendReason::BACKGROUNDED
                             : SuspendReason::SUSPENDED_BY_POLICY;
  player_api_->Suspend(reason);
  delegate_->DidMediaSuspended(delegate_id_);
}

void WebMediaPlayerNeva::OnMediaActivationPermitted() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // If we already have activation permit, just skip.
  if (has_activation_permit_) {
    delegate_->DidMediaActivated(delegate_id_);
    return;
  }

  has_activation_permit_ = true;
  if (is_loading_) {
    OnLoadPermitted();
    return;
  } else if (is_suspended_) {
    OnResume();
    return;
  }

  Play();
  GetClient()->PlaybackStateChanged();
  delegate_->DidMediaActivated(delegate_id_);
}

void WebMediaPlayerNeva::OnResume() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (!is_suspended_) {
    delegate_->DidMediaActivated(delegate_id_);
    return;
  }

  is_suspended_ = false;

  if (HasVideo()) {
    if (RenderTexture())
      video_frame_provider_->SetStorageType(media::VideoFrame::STORAGE_OPAQUE);
#if defined(VIDEO_HOLE)
    else
      video_frame_provider_->SetStorageType(media::VideoFrame::STORAGE_HOLE);
#endif
    video_frame_provider_->UpdateVideoFrame();
  }

  if (!player_api_->IsRecoverableOnResume()) {
    player_api_.reset(MediaPlayerNevaFactory::CreateMediaPlayerNeva(
        this, client_->ContentMIMEType().Latin1(), main_task_runner_, app_id_));
    player_api_->SetVolume(volume_);
    LoadMedia();
    SetDisplayWindow();
  } else {
    player_api_->Resume();
  }

  if (status_on_suspended_ == PlayingStatus) {
    Play();
    GetClient()->PlaybackStateChanged();
    status_on_suspended_ = UnknownStatus;
  }

  delegate_->DidMediaActivated(delegate_id_);
}

void WebMediaPlayerNeva::OnLoadPermitted() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  LOG(ERROR) << __func__;

  FUNC_LOG(1);
  if (!defer_load_cb_.is_null()) {
    defer_load_cb_.Run(
        base::Bind(&WebMediaPlayerNeva::DoLoad, AsWeakPtr(), pending_load_type_,
                   pending_source_.GetAsURL(), pending_cors_mode_));
    return;
  }

  DoLoad(pending_load_type_, pending_source_.GetAsURL(), pending_cors_mode_);
}

bool WebMediaPlayerNeva::UsesIntrinsicSize() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return player_api_->UsesIntrinsicSize();
}

blink::WebString WebMediaPlayerNeva::MediaId() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return blink::WebString::FromUTF8(player_api_->MediaId());
}

bool WebMediaPlayerNeva::HasAudioFocus() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return player_api_->HasAudioFocus();
}

void WebMediaPlayerNeva::SetAudioFocus(bool focus) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  player_api_->SetAudioFocus(focus);
}

bool WebMediaPlayerNeva::HasVisibility() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return player_api_->HasVisibility();
}

void WebMediaPlayerNeva::SetVisibility(bool visibility) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  player_api_->SetVisibility(visibility);
}

void WebMediaPlayerNeva::OnCustomMessage(
    const blink::WebMediaPlayer::MediaEventType media_event_type,
    const std::string& detail) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1) << __func__ << " detail: " << detail;

  if (!detail.empty())
    client_->SendCustomMessage(media_event_type,
                               blink::WebString::FromUTF8(detail));
}

void WebMediaPlayerNeva::OnAudioFocusChanged() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnAudioFocusChanged();
}

void WebMediaPlayerNeva::OnVideoDisplayWindowChange() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1) << " renderTexture: " << RenderTexture();

  if (RenderTexture()) {
    player_api_->SwitchToAutoLayout();
    LOG(INFO) << __func__ << " called SwitchToAutoLayout";
#if defined(VIDEO_HOLE)
  } else {
    SetDisplayWindow();
#endif
  }
}

void WebMediaPlayerNeva::SetRenderMode(blink::WebMediaPlayer::RenderMode mode) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (render_mode_ == mode)
    return;

  render_mode_ = mode;
  if (RenderTexture()) {
    video_frame_provider_->SetStorageType(media::VideoFrame::STORAGE_OPAQUE);
    player_api_->SwitchToAutoLayout();
    LOG(INFO) << __func__ << " called SwitchToAutoLayout";
  } else {
#if defined(VIDEO_HOLE)
    video_frame_provider_->SetStorageType(media::VideoFrame::STORAGE_HOLE);
#endif
  }
}

void WebMediaPlayerNeva::SetDisableAudio(bool disable) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (audio_disabled_ == disable)
    return;
  LOG(INFO) << __func__ << " disable=" << disable;
  player_api_->SetDisableAudio(disable);
}

void WebMediaPlayerNeva::EnabledAudioTracksChanged(
    const blink::WebVector<TrackId>& enabled_track_ids) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  auto it = std::find_if(
      audio_track_ids_.begin(), audio_track_ids_.end(),
      [&enabled_track_ids](const MediaTrackId& id) {
        return enabled_track_ids[enabled_track_ids.size() - 1] == id.first;
      });

  if (it != audio_track_ids_.end())
    player_api_->SelectTrack(MediaTrackType::kAudio, it->second);
}

bool WebMediaPlayerNeva::IsHLSStream() const {
  const GURL& url = redirected_url_.is_empty() ? url_ : redirected_url_;
  return (url.SchemeIsHTTPOrHTTPS() || url.SchemeIsFile()) &&
         url.spec().find("m3u8") != std::string::npos;
}

void WebMediaPlayerNeva::OnMediaSourceOpened(
    blink::WebMediaSource* web_media_source) {
  client_->MediaSourceOpened(web_media_source);
}

void WebMediaPlayerNeva::OnFrameHidden() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (!IsBackgroundedSuspendEnabled())
    return;

  OnSuspend();
}

void WebMediaPlayerNeva::OnFrameShown() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (!IsBackgroundedSuspendEnabled())
    return;

  OnResume();
}

void WebMediaPlayerNeva::OnDidCommitCompositorFrame() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (RenderTexture())
    return;
#if defined(VIDEO_HOLE)
  if (!UpdateBoundaryRectangle()) {
    // UpdateBoundaryRectangle fails when video layer is not in current composition.
    if (HasVisibility()) {
      is_video_offscreen_ = true;
      SetVisibility(false);
    }
    return;
  }
  UpdateVideoHoleBoundary(false);
#endif
}

}  // namespace media
