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

#ifndef MEDIA_BLINK_NEVA_WEBMEDIAPLAYER_NEVA_H_
#define MEDIA_BLINK_NEVA_WEBMEDIAPLAYER_NEVA_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread.h"
#include "base/time/default_tick_clock.h"
#include "base/timer/timer.h"
#include "cc/layers/video_frame_provider.h"
#include "content/public/renderer/render_frame.h"
#include "media/base/audio_renderer_sink.h"
#include "media/base/pipeline.h"
#include "media/base/time_delta_interpolator.h"
#include "media/base/video_frame.h"
#include "media/blink/media_blink_export.h"
#include "media/blink/neva/media_info_loader.h"
#include "media/blink/neva/media_player_neva_interface.h"
#include "media/blink/neva/video_frame_provider_impl.h"
#include "media/blink/webmediaplayer_delegate.h"
#include "third_party/blink/public/platform/web_audio_source_provider.h"
#include "third_party/blink/public/platform/web_float_point.h"
#include "third_party/blink/public/platform/web_media_player.h"
#include "third_party/blink/public/platform/web_media_player_client.h"
#include "third_party/blink/public/platform/web_media_player_source.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "url/gurl.h"

namespace blink {
enum class WebFullscreenVideoStatus;
class WebLocalFrame;
}

namespace cc_blink {
class WebLayerImpl;
}

namespace base {
class SingleThreadTaskRunner;
}

namespace cc {
class VideoLayer;
}

namespace gpu {
struct SyncToken;
}

namespace content {
class StreamTextureFactory;
class RenderThreadImpl;
}  // namespace content

namespace media {
class MediaLog;
class WebMediaPlayerParams;
class WebAudioSourceProviderImpl;

class MEDIA_BLINK_EXPORT WebMediaPlayerNeva
    : public blink::WebMediaPlayer,
      public media::WebMediaPlayerDelegate::Observer,
      public media::MediaPlayerNevaClient,
      public base::MessageLoop::DestructionObserver,
      public base::SupportsWeakPtr<WebMediaPlayerNeva> {
 public:
  enum StatusOnSuspended {
    UnknownStatus = 0,
    PlayingStatus,
    PausedStatus,
  };
  static bool CanSupportMediaType(const std::string& mime);
  static blink::WebMediaPlayer* Create(
      blink::WebLocalFrame* frame,
      blink::WebMediaPlayerClient* client,
      WebMediaPlayerDelegate* delegate,
      const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
      std::unique_ptr<WebMediaPlayerParams> params);

  // This class implements blink::WebMediaPlayer by keeping the private media
  // player api which is supported by target platform
  WebMediaPlayerNeva(
      blink::WebLocalFrame* frame,
      blink::WebMediaPlayerClient* client,
      WebMediaPlayerDelegate* delegate,
      const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
      std::unique_ptr<WebMediaPlayerParams> params);
  ~WebMediaPlayerNeva() override;

  void Load(LoadType load_type,
            const blink::WebMediaPlayerSource& src,
            CORSMode cors_mode) override;

  // Playback controls.
  void Play() override;
  void Pause() override;
  void Seek(double seconds) override;
  void SetRate(double rate) override;
  void SetVolume(double volume) override;

  // TODO(neva): need to check PIP how it shows and implement
  // Enter Picture-in-Picture and notifies Blink with window size
  // when video successfully enters Picture-in-Picture.
  void EnterPictureInPicture(PipWindowOpenedCallback) override {}
  // Exit Picture-in-Picture and notifies Blink when it's done.
  void ExitPictureInPicture(PipWindowClosedCallback) override {}
  // Register a callback that will be run when the Picture-in-Picture window
  // is resized.
  void RegisterPictureInPictureWindowResizeCallback(
      PipWindowResizedCallback) override {}

  blink::WebTimeRanges Buffered() const override;
  blink::WebTimeRanges Seekable() const override;

  // Attempts to switch the audio output device.
  // Implementations of SetSinkId take ownership of the WebSetSinkCallbacks
  // object.
  // Note also that SetSinkId implementations must make sure that all
  // methods of the WebSetSinkCallbacks object, including constructors and
  // destructors, run in the same thread where the object is created
  // (i.e., the blink thread).
  void SetSinkId(const blink::WebString& sinkId,
                 blink::WebSetSinkIdCallbacks*) override {}
  // Methods for painting.
  // FIXME: This path "only works" on Android. It is a workaround for the
  // issue that Skia could not handle Android's GL_TEXTURE_EXTERNAL_OES texture
  // internally. It should be removed and replaced by the normal paint path.
  // https://code.google.com/p/skia/issues/detail?id=1189
  void Paint(blink::WebCanvas* canvas,
             const blink::WebRect& rect,
             cc::PaintFlags& flags,
             int already_uploaded_id,
             VideoFrameUploadMetadata* out_metadata) override;

  // True if the loaded media has a playable video/audio track.
  bool HasVideo() const override;
  bool HasAudio() const override;

  void SetPreload(Preload) override;

  // Dimensions of the video.
  blink::WebSize NaturalSize() const override;

  blink::WebSize VisibleRect() const override;

  // Getters of playback state.
  bool Paused() const override;
  bool Seeking() const override;
  double Duration() const override;
  double CurrentTime() const override;

  // Internal states of loading and network.
  blink::WebMediaPlayer::NetworkState GetNetworkState() const override;
  blink::WebMediaPlayer::ReadyState GetReadyState() const override;

  blink::WebString GetErrorMessage() const override;
  bool DidLoadingProgress() override;

  bool DidGetOpaqueResponseFromServiceWorker() const override;
  bool HasSingleSecurityOrigin() const override;
  bool DidPassCORSAccessCheck() const override;

  double MediaTimeForTimeValue(double timeValue) const override;

  unsigned DecodedFrameCount() const override;
  unsigned DroppedFrameCount() const override;
  size_t AudioDecodedByteCount() const override;
  size_t VideoDecodedByteCount() const override;

  // media::MediaPlayerNevaClinet callback implementation
  void OnMediaMetadataChanged(base::TimeDelta duration,
                              int width,
                              int height,
                              bool success) override;
  void OnLoadComplete() override;
  void OnPlaybackComplete() override;
  void OnBufferingUpdate(int percentage) override;
  // void OnBufferingUpdate(double begin, double end) override;
  void OnSeekComplete(const base::TimeDelta& current_time) override;
  void OnMediaError(int error_type) override;
  void OnVideoSizeChanged(int width, int height) override;

  // Called to update the current time.
  void OnTimeUpdate(base::TimeDelta current_timestamp,
                    base::TimeTicks current_time_ticks) override;

  // Functions called when media player status changes.
  void OnMediaPlayerPlay()
      override;  // TODO(wanchang): need to check if it is required
  void OnMediaPlayerPause()
      override;  // TODO(wanchang): need to check if it is required

  void OnCustomMessage(const blink::WebMediaPlayer::MediaEventType,
                       const std::string& detail) override;

  // Function called when audio focus is actually changed.
  void OnAudioFocusChanged() override;

  void OnVideoDisplayWindowChange() override;

  bool CopyVideoTextureToPlatformTexture(
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
      VideoFrameUploadMetadata* out_metadata) override;

  bool SupportsOverlayFullscreenVideo() override;
  void EnteredFullscreen() override;
  void ExitedFullscreen() override;
  void BecameDominantVisibleContent(bool is_dominant) override;
  void SetIsEffectivelyFullscreen(blink::WebFullscreenVideoStatus) override;

  // WebMediaPlayerDelegate::Observer interface stubs
  void OnFrameHidden() override;
  void OnFrameClosed() override {}
  void OnFrameShown() override;
  void OnIdleTimeout() override {}
  void OnPlay() override {}
  void OnPause() override {}
  void OnSeekForward(double seconds) override {}
  void OnSeekBackward(double seconds) override {}
  void OnVolumeMultiplierUpdate(double multiplier) override {}
  void OnBecamePersistentVideo(bool value) override {}
  void OnPictureInPictureModeEnded() override {}
  void OnDidCommitCompositorFrame() override;
  void OnSuspend() override;
  void OnMediaActivationPermitted() override;
  void OnResume();
  void OnLoadPermitted();

  blink::WebAudioSourceProvider* GetAudioSourceProvider() override;

  // As we are closing the tab or even the browser, |main_loop_| is destroyed
  // even before this object gets destructed, so we need to know when
  // |main_loop_| is being destroyed and we can stop posting repaint task
  // to it.
  void WillDestroyCurrentMessageLoop() override;

  void OnActiveRegionChanged(const blink::WebRect&) override;

  void Repaint();

  void SetOpaque(bool opaque);

  bool UseVideoTexture();

  // neva::WebMediaPlayer implementaions
  bool UsesIntrinsicSize() const override;
  blink::WebString MediaId() const override;
  bool HasAudioFocus() const override;
  void SetAudioFocus(bool focus) override;
  bool HasVisibility() const override;
  void SetVisibility(bool) override;
  void SetRenderMode(blink::WebMediaPlayer::RenderMode mode) override;

  void EnabledAudioTracksChanged(
      const blink::WebVector<TrackId>& enabledTrackIds) override;

  bool RenderTexture() {
    return render_mode_ == blink::WebMediaPlayer::RenderModeTexture;
  }

 private:
  // void OnPipelinePlaybackStateChanged(bool playing);
  void UpdatePlayingState(bool is_playing);

  // Helpers that set the network/ready state and notifies the client if
  // they've changed.
  void UpdateNetworkState(blink::WebMediaPlayer::NetworkState state);
  void UpdateReadyState(blink::WebMediaPlayer::ReadyState state);

  bool IsHLSStream() const;

  void DoLoad(LoadType load_type, const blink::WebURL& url, CORSMode cors_mode);
  void DidLoadMediaInfo(bool ok, const GURL& redirected_url);
  void LoadMedia();

  // Called after asynchronous initialization of a data source completed.
  void DataSourceInitialized(const GURL& gurl, bool success);

  // Called when the data source is downloading or paused.
  void NotifyDownloading(bool is_downloading);

#if defined(VIDEO_HOLE)
  void UpdateVideoHoleBoundary(bool forced = false);

  // Calculate the boundary rectangle of the media player (i.e. location and
  // size of the video frame).
  // Returns true if the geometry has been changed since the last call.
  bool UpdateBoundaryRectangle();
#endif  // defined(VIDEO_HOLE)

  // Getter method to |client_|.
  blink::WebMediaPlayerClient* GetClient();

  // for MSE implementation
  void OnMediaSourceOpened(blink::WebMediaSource* web_media_source);

  // Called when a decoder detects that the key needed to decrypt the stream
  // is not available.
  // void OnWaitingForDecryptionKey() override;
  // end of MSE implementation
  //-----------------------------------------------------

  blink::WebLocalFrame* frame_;

  blink::WebMediaPlayerClient* client_;

  // WebMediaPlayer notifies the |delegate_| of playback state changes using
  // |delegate_id_|; an id provided after registering with the delegate.  The
  // WebMediaPlayer may also receive directives (play, pause) from the delegate
  // via the WebMediaPlayerDelegate::Observer interface after registration.
  media::WebMediaPlayerDelegate* delegate_;
  int delegate_id_;

  // Callback responsible for determining if loading of media should be deferred
  // for external reasons; called during load().
  base::Callback<void(const base::Closure&)> defer_load_cb_;

  // Save the list of buffered time ranges.
  blink::WebTimeRanges buffered_;

  // Size of the video.
  blink::WebSize natural_size_;

  // The video frame object used for rendering by the compositor.
  scoped_refptr<media::VideoFrame> current_frame_;
  base::Lock current_frame_lock_;

  base::ThreadChecker main_thread_checker_;

  // Message loop for media thread.
  scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;

  // URL of the media file to be fetched.
  GURL url_;

  // URL of the media file after |media_info_loader_| resolves all the
  // redirections.
  GURL redirected_url_;

  // Media duration.
  base::TimeDelta duration_;

  // Seek gets pending if another seek is in progress. Only last pending seek
  // will have effect.
  bool pending_seek_;
  base::TimeDelta pending_seek_time_;

  // Internal seek state.
  bool seeking_;
  base::TimeDelta seek_time_;

  // Whether loading has progressed since the last call to didLoadingProgress.
  bool did_loading_progress_;

  // Private MediaPlayer API instance
  std::unique_ptr<MediaPlayerNeva> player_api_;

  // TODO(hclam): get rid of these members and read from the pipeline directly.
  blink::WebMediaPlayer::NetworkState network_state_;
  blink::WebMediaPlayer::ReadyState ready_state_;

  // Whether the media player is playing.
  bool is_playing_;

  // Whether the video size info is available.
  bool has_size_info_;

  // A pointer back to the compositor to inform it about state changes. This is
  // not NULL while the compositor is actively using this webmediaplayer.
  // Accessed on main thread and on compositor thread when main thread is
  // blocked.
  cc::VideoFrameProvider::Client* video_frame_provider_client_;

  // The compositor layer for displaying the video content when using composited
  // playback.
  scoped_refptr<cc::VideoLayer> video_layer_;

#if defined(VIDEO_HOLE)
  // A rectangle represents the geometry of video frame, when computed last
  // time.
  gfx::Rect last_computed_rect_in_view_space_;

  // Whether to use the video overlay for all embedded video.
  // True only for testing.
  bool force_use_overlay_embedded_video_;
#endif  // defined(VIDEO_HOLE)

  std::unique_ptr<MediaLog> media_log_;

  std::unique_ptr<MediaInfoLoader> info_loader_;

  // base::TickClock used by |interpolator_|.
  base::DefaultTickClock default_tick_clock_;

  // Tracks the most recent media time update and provides interpolated values
  // as playback progresses.
  TimeDeltaInterpolator interpolator_;

  // TODO(wanchang): fix it
  // std::unique_ptr<MediaSourceDelegate> media_source_delegate_;

  // Whether OnPlaybackComplete() has been called since the last playback.
  bool playback_completed_;

  // Message loops for posting tasks on Chrome's main thread. Also used
  // for DCHECKs so methods calls won't execute in the wrong thread.
  base::MessageLoop* main_loop_;

  base::TimeDelta paused_time_;

  scoped_refptr<WebAudioSourceProviderImpl> audio_source_provider_;

  bool is_suspended_;
  StatusOnSuspended status_on_suspended_;

  base::RepeatingTimer paintTimer_;

  bool SelectTrack(std::string& type, int32_t index);
  std::vector<WebMediaPlayer::TrackId> audio_track_ids_;

  const scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;
  std::unique_ptr<VideoFrameProviderImpl> video_frame_provider_;
  blink::WebMediaPlayer::RenderMode render_mode_;

  base::OneShotTimer throttle_update_video_hole_boundary_;

  float content_position_offset_;

  gfx::Rect visible_rect_in_screen_space_;
  gfx::Rect source_rect_in_video_space_;
  const blink::WebFloatPoint additional_contents_scale_;
  bool is_fullscreen_;

  blink::WebRect active_video_region_;
  bool active_video_region_changed_;
  bool is_video_offscreen_;

  blink::WebRect ScaleWebRect(const blink::WebRect& rect,
                              blink::WebFloatPoint scale);
  std::string app_id_;

  bool is_loading_;
  LoadType pending_load_type_;
  blink::WebMediaPlayerSource pending_source_;
  CORSMode pending_cors_mode_;

  bool has_activation_permit_ = false;

  DISALLOW_COPY_AND_ASSIGN(WebMediaPlayerNeva);
};

}  // namespace media

#endif  // MEDIA_BLINK_NEVA_WEBMEDIAPLAYER_NEVA_H_
