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

#include "components/cdm/common/neva/cdm_info_util.h"

#include "base/containers/flat_set.h"
#include "base/files/file_util.h"
#include "base/version.h"
#include "content/public/common/cdm_info.h"
#include "media/base/decrypt_config.h"
#include "media/base/video_codecs.h"

#include "widevine_cdm_version.h"  // In SHARED_INTERMEDIATE_DIR.

namespace cdm {

#if defined(WIDEVINE_CDM_AVAILABLE)
bool IsWidevineAvailable(
    base::FilePath& cdm_path,
    std::vector<media::VideoCodec>& codecs_supported,
    bool& supports_persistent_license,
    base::flat_set<media::EncryptionMode>& modes_supported) {
  static enum {
    NOT_CHECKED,
    FOUND,
    NOT_FOUND,
  } widevine_cdm_file_check = NOT_CHECKED;

  // TODO(neva): Get file path from configuration
  const std::string default_libwidevinecdm_path = "/usr/lib/libwidevinecdm.so";
  cdm_path = base::FilePath::FromUTF8Unsafe(default_libwidevinecdm_path);

  if (widevine_cdm_file_check == NOT_CHECKED)
    widevine_cdm_file_check = base::PathExists(cdm_path) ? FOUND : NOT_FOUND;

  if (widevine_cdm_file_check == FOUND) {
    codecs_supported.push_back(media::VideoCodec::kCodecVP8);
    codecs_supported.push_back(media::VideoCodec::kCodecVP9);
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
    codecs_supported.push_back(media::VideoCodec::kCodecH264);
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)

    supports_persistent_license = false;

    modes_supported.insert(media::EncryptionMode::kCenc);

    return true;
  }

  return false;
}
#endif  // defined(WIDEVINE_CDM_AVAILABLE)

void AddContentDecryptionModules(std::vector<content::CdmInfo>& cdms) {
#if defined(WIDEVINE_CDM_AVAILABLE)
  base::FilePath cdm_path;
  std::vector<media::VideoCodec> video_codecs_supported;
  bool supports_persistent_license = false;
  base::flat_set<media::EncryptionMode> encryption_modes_supported;
  if (IsWidevineAvailable(cdm_path, video_codecs_supported,
                          supports_persistent_license,
                          encryption_modes_supported)) {
    const base::Version version(WIDEVINE_CDM_VERSION_STRING);
    DCHECK(version.IsValid());

    cdms.push_back(content::CdmInfo(
        kWidevineCdmDisplayName, kWidevineCdmGuid, version, cdm_path,
        kWidevineCdmFileSystemId, video_codecs_supported,
        supports_persistent_license, encryption_modes_supported,
        kWidevineKeySystem, false));
  }
#endif  // defined(WIDEVINE_CDM_AVAILABLE)
}

}  // namespace cdm
