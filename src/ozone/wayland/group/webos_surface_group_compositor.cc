// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#include "ozone/wayland/display.h"
#include "ozone/wayland/group/webos_surface_group.h"
#include "ozone/wayland/group/webos_surface_group_compositor.h"
#include "ozone/wayland/shell/shell_surface.h"
#include "ozone/wayland/window.h"

namespace ozonewayland {

WebOSSurfaceGroupCompositor::WebOSSurfaceGroupCompositor(wl_registry* registry,
                                                         uint32_t id)
    : wl_webos_surface_group_compositor(registry, id) {}

WebOSSurfaceGroupCompositor::~WebOSSurfaceGroupCompositor() {}

WebOSSurfaceGroup* WebOSSurfaceGroupCompositor::CreateGroup(
    unsigned handle,
    const std::string& name) {
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  WaylandWindow* window = display->GetWindow(handle);
  struct ::wl_webos_surface_group* grp =
      create_surface_group(window->ShellSurface()->GetWLSurface(), name);
  if (!grp)
    return NULL;

  WebOSSurfaceGroup* group = new WebOSSurfaceGroup(handle);
  group->init(grp);
  return group;
}

WebOSSurfaceGroup* WebOSSurfaceGroupCompositor::GetGroup(
    unsigned handle,
    const std::string& name) {
  struct ::wl_webos_surface_group* grp = get_surface_group(name);
  if (!grp)
    return NULL;

  WebOSSurfaceGroup* group = new WebOSSurfaceGroup(handle);
  group->init(grp);
  return group;
}

}  // namcespace ozonewayland
