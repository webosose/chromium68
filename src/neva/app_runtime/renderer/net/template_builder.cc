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

#include "neva/app_runtime/renderer/net/template_builder.h"

#include "base/json/json_file_value_serializer.h"
#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "neva/app_runtime/grit/network_error_pages.h"
#include "ui/base/layout.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/template_expressions.h"

using namespace ui;

namespace app_runtime {
// Appends the source for i18n Templates in a script tag.
void AppendI18nTemplateSourceHtml(std::string* output) {
  base::StringPiece err_page_template(
      ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_APP_RUNTIME_NETWORK_ERROR_PAGE_TEMPLATE_JS));

  output->append("<script>");
  err_page_template.AppendToString(output);
  output->append("</script>");
}

void AppendJsonJS(const base::DictionaryValue* json, std::string* output) {
  // Convert the template data to a json string.
  DCHECK(json) << "must include json data structure";

  std::string jstext;
  JSONStringValueSerializer serializer(&jstext);
  serializer.Serialize(*json);
  output->append("error_info = ");
  output->append(jstext);
  output->append(";");
}

void AppendJsonHtml(const base::DictionaryValue* json, std::string* output) {
  std::string javascript_string;
  AppendJsonJS(json, &javascript_string);

  // </ confuses the HTML parser because it could be a </script> tag.  So we
  // replace </ with <\/.  The extra \ will be ignored by the JS engine.
  base::ReplaceSubstringsAfterOffset(&javascript_string, 0, "</", "<\\/");

  output->append("<script>");
  output->append(javascript_string);
  output->append("</script>");
}

std::string GetTemplatesHtml(const base::StringPiece& html_template,
                             const base::DictionaryValue* json,
                             int err_code,
                             const base::StringPiece& template_id,
                             int viewport_width,
                             int viewport_height) {
  std::string output(html_template.data(), html_template.size());

  output.append("<script>");
  output.append("var errorCode = ");
  output.append(std::to_string(err_code));
  output.append(";");

  output.append("var appResolutionWidth = ");
  output.append(std::to_string(viewport_width));
  output.append(";");

  output.append("var appResolutionHeight = ");
  output.append(std::to_string(viewport_height));
  output.append(";");
  output.append("</script>");

  AppendJsonHtml(json, &output);
  AppendI18nTemplateSourceHtml(&output);

  return output;
}

}  // namespace app_runtime
