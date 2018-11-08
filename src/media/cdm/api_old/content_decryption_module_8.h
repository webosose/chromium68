// Copyright 2018 LG Electronics, Inc.
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

#ifndef MEDIA_CDM_API_OLD_CONTENT_DECRYPTION_MODULE_8_H_
#define MEDIA_CDM_API_OLD_CONTENT_DECRYPTION_MODULE_8_H_

#include "media/cdm/api/content_decryption_module.h"

// Data types and interfaces for content_decryption_module v8.
// This file was deperecated in upstream chromium, but we added this
// for integrating with existing media components in webOS.
// NOTE: DO NOT MODIFY THIS HEADER ONLY. Many subcomponents such as
// widevine-modular or mediadrm depend on definitions in this header.

// Maybe the best place for this header can be media/cdm/api. But the
// path is managed as own repository(git) in upstream chromium. So we
// added this file at here for managing easily.

namespace cdm {
// This must at least contain the exceptions defined in the spec:
// https://w3c.github.io/encrypted-media/#exceptions
// The following starts with the list of DOM4 exceptions from:
// http://www.w3.org/TR/dom/#domexception
// Some DOM4 exceptions are not included as they are not expected to be used.
enum Error {
  kNotSupportedError = 9,
  kInvalidStateError = 11,
  kInvalidAccessError = 15,
  kQuotaExceededError = 22,

  // Additional exceptions that do not have assigned codes.
  // There are other non-EME-specific values, not included in this list.
  kUnknownError = 30,

  // Additional values from previous EME versions. They currently have no
  // matching DOMException.
  kClientError = 100,
  kOutputError = 101
};

struct InputBuffer {
  const uint8_t* data;  // Pointer to the beginning of the input data.
  uint32_t data_size;   // Size (in bytes) of |data|.

  const uint8_t* key_id;  // Key ID to identify the decryption key.
  uint32_t key_id_size;   // Size (in bytes) of |key_id|.

  const uint8_t* iv;  // Initialization vector.
  uint32_t iv_size;   // Size (in bytes) of |iv|.

  const struct SubsampleEntry* subsamples;
  uint32_t num_subsamples;  // Number of subsamples in |subsamples|.

  int64_t timestamp;  // Presentation timestamp in microseconds.

  uint32_t is_video;  // Indicates if the buffer contains video data.
};

struct AudioDecoderConfig {
  AudioCodec codec;
  int32_t channel_count;
  int32_t bits_per_channel;
  int32_t samples_per_second;

  // Optional byte data required to initialize audio decoders, such as the
  // vorbis setup header.
  uint8_t* extra_data;
  uint32_t extra_data_size;
};

// TODO(neva): describe comment
struct VideoDecoderConfig {
  VideoCodec codec;
  VideoCodecProfile profile;
  VideoFormat format;

  // Width and height of video frame immediately post-decode. Not all pixels
  // in this region are valid.
  Size coded_size;

  // Optional byte data required to initialize video decoders, such as H.264
  // AAVC data.
  uint8_t* extra_data;
  uint32_t extra_data_size;
};

class CDM_CLASS_API Host_8;

// ContentDecryptionModule interface that all CDMs need to implement.
// The interface is versioned for backward compatibility.
// Note: ContentDecryptionModule implementations must use the allocator
// provided in CreateCdmInstance() to allocate any Buffer that needs to
// be passed back to the caller. Implementations must call Buffer::Destroy()
// when a Buffer is created that will never be returned to the caller.
class CDM_CLASS_API ContentDecryptionModule_8 {
 public:
  static const int kVersion = 8;
  typedef Host_8 Host;

  // Initializes the CDM instance, providing information about permitted
  // functionalities.
  // If |allow_distinctive_identifier| is false, messages from the CDM,
  // such as message events, must not contain a Distinctive Identifier,
  // even in an encrypted form.
  // If |allow_persistent_state| is false, the CDM must not attempt to
  // persist state. Calls to CreateFileIO() will fail.
  virtual void Initialize(bool allow_distinctive_identifier,
                          bool allow_persistent_state) = 0;

  // SetServerCertificate(), CreateSessionAndGenerateRequest(), LoadSession(),
  // UpdateSession(), CloseSession(), and RemoveSession() all accept a
  // |promise_id|, which must be passed to the completion Host method
  // (e.g. Host::OnResolveNewSessionPromise()).

  // Gets the key status if the CDM has a hypothetical key with the |policy|.
  // The CDM must respond by calling either Host::OnResolveKeyStatusPromise()
  // with the result key status or Host::OnRejectPromise() if an unexpected
  // error happened or this method is not supported.
  virtual void GetStatusForPolicy(uint32_t promise_id, const Policy& policy) {}

  // Provides a server certificate to be used to encrypt messages to the
  // license server. The CDM must respond by calling either
  // Host::OnResolvePromise() or Host::OnRejectPromise().
  virtual void SetServerCertificate(uint32_t promise_id,
                                    const uint8_t* server_certificate_data,
                                    uint32_t server_certificate_data_size) = 0;

  // Creates a session given |session_type|, |init_data_type|, and |init_data|.
  // The CDM must respond by calling either Host::OnResolveNewSessionPromise()
  // or Host::OnRejectPromise().
  virtual void CreateSessionAndGenerateRequest(uint32_t promise_id,
                                               SessionType session_type,
                                               InitDataType init_data_type,
                                               const uint8_t* init_data,
                                               uint32_t init_data_size) = 0;

  // Loads the session of type |session_type| specified by |session_id|.
  // The CDM must respond by calling either Host::OnResolveNewSessionPromise()
  // or Host::OnRejectPromise(). If the session is not found, call
  // Host::OnResolveNewSessionPromise() with session_id = NULL.
  virtual void LoadSession(uint32_t promise_id,
                           SessionType session_type,
                           const char* session_id,
                           uint32_t session_id_size) = 0;

  // Updates the session with |response|. The CDM must respond by calling
  // either Host::OnResolvePromise() or Host::OnRejectPromise().
  virtual void UpdateSession(uint32_t promise_id,
                             const char* session_id,
                             uint32_t session_id_size,
                             const uint8_t* response,
                             uint32_t response_size) = 0;

  // Requests that the CDM close the session. The CDM must respond by calling
  // either Host::OnResolvePromise() or Host::OnRejectPromise() when the request
  // has been processed. This may be before the session is closed. Once the
  // session is closed, Host::OnSessionClosed() must also be called.
  virtual void CloseSession(uint32_t promise_id,
                            const char* session_id,
                            uint32_t session_id_size) = 0;

  // Removes any stored session data associated with this session. Will only be
  // called for persistent sessions. The CDM must respond by calling either
  // Host::OnResolvePromise() or Host::OnRejectPromise() when the request has
  // been processed.
  virtual void RemoveSession(uint32_t promise_id,
                             const char* session_id,
                             uint32_t session_id_size) = 0;

  // Performs scheduled operation with |context| when the timer fires.
  virtual void TimerExpired(void* context) = 0;

  // Decrypts the |encrypted_buffer|.
  //
  // Returns kSuccess if decryption succeeded, in which case the callee
  // should have filled the |decrypted_buffer| and passed the ownership of
  // |data| in |decrypted_buffer| to the caller.
  // Returns kNoKey if the CDM did not have the necessary decryption key
  // to decrypt.
  // Returns kDecryptError if any other error happened.
  // If the return value is not kSuccess, |decrypted_buffer| should be ignored
  // by the caller.
  virtual Status Decrypt(const InputBuffer& encrypted_buffer,
                         DecryptedBlock* decrypted_buffer) = 0;

  // Initializes the CDM audio decoder with |audio_decoder_config|. This
  // function must be called before DecryptAndDecodeSamples() is called.
  //
  // Returns kSuccess if the |audio_decoder_config| is supported and the CDM
  // audio decoder is successfully initialized.
  // Returns kSessionError if |audio_decoder_config| is not supported. The CDM
  // may still be able to do Decrypt().
  // Returns kDeferredInitialization if the CDM is not ready to initialize the
  // decoder at this time. Must call Host::OnDeferredInitializationDone() once
  // initialization is complete.
  virtual Status InitializeAudioDecoder(
      const AudioDecoderConfig& audio_decoder_config) = 0;

  // Initializes the CDM video decoder with |video_decoder_config|. This
  // function must be called before DecryptAndDecodeFrame() is called.
  //
  // Returns kSuccess if the |video_decoder_config| is supported and the CDM
  // video decoder is successfully initialized.
  // Returns kSessionError if |video_decoder_config| is not supported. The CDM
  // may still be able to do Decrypt().
  // Returns kDeferredInitialization if the CDM is not ready to initialize the
  // decoder at this time. Must call Host::OnDeferredInitializationDone() once
  // initialization is complete.
  virtual Status InitializeVideoDecoder(
      const VideoDecoderConfig& video_decoder_config) = 0;

  // De-initializes the CDM decoder and sets it to an uninitialized state. The
  // caller can initialize the decoder again after this call to re-initialize
  // it. This can be used to reconfigure the decoder if the configuration
  // changes.
  virtual void DeinitializeDecoder(StreamType decoder_type) = 0;

  // Resets the CDM decoder to an initialized clean state. All internal buffers
  // MUST be flushed.
  virtual void ResetDecoder(StreamType decoder_type) = 0;

  // Decrypts the |encrypted_buffer| and decodes the decrypted buffer into a
  // |video_frame|. Upon end-of-stream, the caller should call this function
  // repeatedly with empty |encrypted_buffer| (|data| == NULL) until only empty
  // |video_frame| (|format| == kEmptyVideoFrame) is produced.
  //
  // Returns kSuccess if decryption and decoding both succeeded, in which case
  // the callee will have filled the |video_frame| and passed the ownership of
  // |frame_buffer| in |video_frame| to the caller.
  // Returns kNoKey if the CDM did not have the necessary decryption key
  // to decrypt.
  // Returns kNeedMoreData if more data was needed by the decoder to generate
  // a decoded frame (e.g. during initialization and end-of-stream).
  // Returns kDecryptError if any decryption error happened.
  // Returns kDecodeError if any decoding error happened.
  // If the return value is not kSuccess, |video_frame| should be ignored by
  // the caller.
  virtual Status DecryptAndDecodeFrame(const InputBuffer& encrypted_buffer,
                                       VideoFrame* video_frame) = 0;

  // Decrypts the |encrypted_buffer| and decodes the decrypted buffer into
  // |audio_frames|. Upon end-of-stream, the caller should call this function
  // repeatedly with empty |encrypted_buffer| (|data| == NULL) until only empty
  // |audio_frames| is produced.
  //
  // Returns kSuccess if decryption and decoding both succeeded, in which case
  // the callee will have filled |audio_frames| and passed the ownership of
  // |data| in |audio_frames| to the caller.
  // Returns kNoKey if the CDM did not have the necessary decryption key
  // to decrypt.
  // Returns kNeedMoreData if more data was needed by the decoder to generate
  // audio samples (e.g. during initialization and end-of-stream).
  // Returns kDecryptError if any decryption error happened.
  // Returns kDecodeError if any decoding error happened.
  // If the return value is not kSuccess, |audio_frames| should be ignored by
  // the caller.
  virtual Status DecryptAndDecodeSamples(const InputBuffer& encrypted_buffer,
                                         AudioFrames* audio_frames) = 0;

  // Called by the host after a platform challenge was initiated via
  // Host::SendPlatformChallenge().
  virtual void OnPlatformChallengeResponse(
      const PlatformChallengeResponse& response) = 0;

  // Called by the host after a call to Host::QueryOutputProtectionStatus(). The
  // |link_mask| is a bit mask of OutputLinkTypes and |output_protection_mask|
  // is a bit mask of OutputProtectionMethods. If |result| is kQueryFailed,
  // then |link_mask| and |output_protection_mask| are undefined and should
  // be ignored.
  virtual void OnQueryOutputProtectionStatus(
      QueryResult result,
      uint32_t link_mask,
      uint32_t output_protection_mask) = 0;

  // Destroys the object in the same context as it was created.
  virtual void Destroy() = 0;

 protected:
  ContentDecryptionModule_8() {}
  virtual ~ContentDecryptionModule_8() {}
};

class CDM_CLASS_API Host_8 {
 public:
  static const int kVersion = 8;

  // Returns a Buffer* containing non-zero members upon success, or NULL on
  // failure. The caller owns the Buffer* after this call. The buffer is not
  // guaranteed to be zero initialized. The capacity of the allocated Buffer
  // is guaranteed to be not less than |capacity|.
  virtual Buffer* Allocate(uint32_t capacity) = 0;

  // Requests the host to call ContentDecryptionModule::TimerFired() |delay_ms|
  // from now with |context|.
  virtual void SetTimer(int64_t delay_ms, void* context) = 0;

  // Returns the current wall time in seconds.
  virtual Time GetCurrentWallTime() = 0;

  // Called by the CDM when a session is created or loaded and the value for the
  // MediaKeySession's sessionId attribute is available (|session_id|).
  // This must be called before OnSessionMessage() or
  // OnSessionKeysChange() is called for the same session. |session_id_size|
  // should not include null termination.
  // When called in response to LoadSession(), the |session_id| must be the
  // same as the |session_id| passed in LoadSession(), or NULL if the
  // session could not be loaded.
  virtual void OnResolveNewSessionPromise(uint32_t promise_id,
                                          const char* session_id,
                                          uint32_t session_id_size) = 0;

  // Called by the CDM when a session is updated or released.
  virtual void OnResolvePromise(uint32_t promise_id) = 0;

  // Called by the CDM when an error occurs as a result of one of the
  // ContentDecryptionModule calls that accept a |promise_id|.
  // |error| must be specified, |error_message| and |system_code|
  // are optional. |error_message_size| should not include null termination.
  virtual void OnRejectPromise(uint32_t promise_id,
                               Error error,
                               uint32_t system_code,
                               const char* error_message,
                               uint32_t error_message_size) = 0;

  // Called by the CDM when it has a message for session |session_id|.
  // Size parameters should not include null termination.
  // |legacy_destination_url| is only for supporting the prefixed EME API and
  // is ignored by unprefixed EME. It should only be non-null if |message_type|
  // is kLicenseRenewal.
  virtual void OnSessionMessage(const char* session_id,
                                uint32_t session_id_size,
                                MessageType message_type,
                                const char* message,
                                uint32_t message_size,
                                const char* legacy_destination_url,
                                uint32_t legacy_destination_url_length) = 0;

  // Called by the CDM when there has been a change in keys or their status for
  // session |session_id|. |has_additional_usable_key| should be set if a
  // key is newly usable (e.g. new key available, previously expired key has
  // been renewed, etc.) and the browser should attempt to resume playback.
  // |key_ids| is the list of key ids for this session along with their
  // current status. |key_ids_count| is the number of entries in |key_ids|.
  // Size parameter for |session_id| should not include null termination.
  virtual void OnSessionKeysChange(const char* session_id,
                                   uint32_t session_id_size,
                                   bool has_additional_usable_key,
                                   const KeyInformation* keys_info,
                                   uint32_t keys_info_count) = 0;

  // Called by the CDM when there has been a change in the expiration time for
  // session |session_id|. This can happen as the result of an Update() call
  // or some other event. If this happens as a result of a call to Update(),
  // it must be called before resolving the Update() promise. |new_expiry_time|
  // can be 0 to represent "undefined". Size parameter should not include
  // null termination.
  virtual void OnExpirationChange(const char* session_id,
                                  uint32_t session_id_size,
                                  Time new_expiry_time) = 0;

  // Called by the CDM when session |session_id| is closed. Size
  // parameter should not include null termination.
  virtual void OnSessionClosed(const char* session_id,
                               uint32_t session_id_size) = 0;

  // Called by the CDM when an error occurs in session |session_id|
  // unrelated to one of the ContentDecryptionModule calls that accept a
  // |promise_id|. |error| must be specified, |error_message| and
  // |system_code| are optional. Length parameters should not include null
  // termination.
  // Note:
  // - This method is only for supporting prefixed EME API.
  // - This method will be ignored by unprefixed EME. All errors reported
  //   in this method should probably also be reported by one of other methods.
  virtual void OnLegacySessionError(const char* session_id,
                                    uint32_t session_id_length,
                                    Error error,
                                    uint32_t system_code,
                                    const char* error_message,
                                    uint32_t error_message_length) = 0;

  // The following are optional methods that may not be implemented on all
  // platforms.

  // Sends a platform challenge for the given |service_id|. |challenge| is at
  // most 256 bits of data to be signed. Once the challenge has been completed,
  // the host will call ContentDecryptionModule::OnPlatformChallengeResponse()
  // with the signed challenge response and platform certificate. Size
  // parameters should not include null termination.
  virtual void SendPlatformChallenge(const char* service_id,
                                     uint32_t service_id_size,
                                     const char* challenge,
                                     uint32_t challenge_size) = 0;

  // Attempts to enable output protection (e.g. HDCP) on the display link. The
  // |desired_protection_mask| is a bit mask of OutputProtectionMethods. No
  // status callback is issued, the CDM must call QueryOutputProtectionStatus()
  // periodically to ensure the desired protections are applied.
  virtual void EnableOutputProtection(uint32_t desired_protection_mask) = 0;

  // Requests the current output protection status. Once the host has the status
  // it will call ContentDecryptionModule::OnQueryOutputProtectionStatus().
  virtual void QueryOutputProtectionStatus() = 0;

  // Must be called by the CDM if it returned kDeferredInitialization during
  // InitializeAudioDecoder() or InitializeVideoDecoder().
  virtual void OnDeferredInitializationDone(StreamType stream_type,
                                            Status decoder_status) = 0;

  // Creates a FileIO object from the host to do file IO operation. Returns NULL
  // if a FileIO object cannot be obtained. Once a valid FileIO object is
  // returned, |client| must be valid until FileIO::Close() is called. The
  // CDM can call this method multiple times to operate on different files.
  virtual FileIO* CreateFileIO(FileIOClient* client) = 0;

 protected:
  Host_8() {}
  virtual ~Host_8() {}
};

}  // namespace cdm

#endif  // MEDIA_CDM_API_OLD_CONTENT_DECRYPTION_MODULE_8_H_
