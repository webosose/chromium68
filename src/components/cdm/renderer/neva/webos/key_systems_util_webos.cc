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

#include "components/cdm/renderer/neva/key_systems_util.h"

#include "components/cdm/renderer/widevine_key_system_properties.h"
#include "content/public/renderer/key_system_support.h"
#include "media/base/eme_constants.h"
#include "media/base/video_codecs.h"

#include "widevine_cdm_version.h"  // In SHARED_INTERMEDIATE_DIR.

#if defined(WIDEVINE_CDM_AVAILABLE) && defined(WIDEVINE_CDM_MIN_GLIBC_VERSION)
#include <gnu/libc-version.h>
#include "base/version.h"
#endif

namespace cdm {

#if defined(WIDEVINE_CDM_AVAILABLE)
void AddWidevineSupportedKeySystems(
    std::vector<std::unique_ptr<media::KeySystemProperties>>& key_systems) {
#if defined(WIDEVINE_CDM_MIN_GLIBC_VERSION)
  base::Version glibc_version(gnu_get_libc_version());
  DCHECK(glibc_version.IsValid());
  if (glibc_version < base::Version(WIDEVINE_CDM_MIN_GLIBC_VERSION)) {
    LOG(ERROR) << __func__ << " glibc version is too old.";
    return;
  }
#endif  // defined(WIDEVINE_CDM_MIN_GLIBC_VERSION)

  std::vector<media::VideoCodec> supported_video_codecs;
  bool supports_persistent_license = false;
  std::vector<media::EncryptionMode> supported_encryption_schemes;
  if (!content::IsKeySystemSupported(
          kWidevineKeySystem, &supported_video_codecs,
          &supports_persistent_license, &supported_encryption_schemes)) {
    LOG(ERROR) << __func__ << " Widevine CDM is not currently available.";
    return;
  }

  using media::SupportedCodecs;
  SupportedCodecs supported_codecs = media::EME_CODEC_NONE;

  // Audio webm codecs are always supported.
  supported_codecs |= media::EME_CODEC_WEBM_OPUS;
  supported_codecs |= media::EME_CODEC_WEBM_VORBIS;

#if BUILDFLAG(USE_PROPRIETARY_CODECS)
  supported_codecs |= media::EME_CODEC_MP4_AAC;
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)

  // Video codecs are determined by what was registered for the CDM.
  for (const auto& codec : supported_video_codecs) {
    switch (codec) {
      case media::VideoCodec::kCodecVP8:
        supported_codecs |= media::EME_CODEC_WEBM_VP8;
        break;
      case media::VideoCodec::kCodecVP9:
        supported_codecs |= media::EME_CODEC_WEBM_VP9;
        supported_codecs |= media::EME_CODEC_COMMON_VP9;
        break;
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
      case media::VideoCodec::kCodecH264:
        supported_codecs |= media::EME_CODEC_MP4_AVC1;
        break;
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)
      default:
        DVLOG(1) << "Unexpected supported codec: " << GetCodecName(codec);
        break;
    }
  }

  using Robustness = cdm::WidevineKeySystemProperties::Robustness;

  using media::EmeSessionTypeSupport;

  using media::EmeFeatureSupport;

  key_systems.emplace_back(new cdm::WidevineKeySystemProperties(
      supported_encryption_schemes, supported_codecs,
      Robustness::HW_SECURE_CRYPTO,  // Maximum audio robustness.
      Robustness::HW_SECURE_ALL,     // Maximum video robustness.
      EmeSessionTypeSupport::SUPPORTED_WITH_IDENTIFIER,  // persistent-license.
      EmeSessionTypeSupport::NOT_SUPPORTED,  // persistent-release-message.
      EmeFeatureSupport::ALWAYS_ENABLED,     // Persistent state.
      EmeFeatureSupport::REQUESTABLE));      // Distinctive identifier.
}
#endif  // defined(WIDEVINE_CDM_AVAILABLE)

void AddSupportedKeySystems(
    std::vector<std::unique_ptr<media::KeySystemProperties>>& key_systems) {
#if defined(WIDEVINE_CDM_AVAILABLE)
  AddWidevineSupportedKeySystems(key_systems);
#endif  // defined(WIDEVINE_CDM_AVAILABLE)
}

}  // namespace cdm
