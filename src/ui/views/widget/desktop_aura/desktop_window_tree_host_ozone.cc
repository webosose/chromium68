// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#if defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
#include "ui/views/widget/desktop_aura/desktop_factory_ozone.h"
#endif
#include "ui/views/widget/desktop_aura/desktop_window_tree_host.h"

namespace views {

DesktopWindowTreeHost* DesktopWindowTreeHost::Create(
    internal::NativeWidgetDelegate* native_widget_delegate,
    DesktopNativeWidgetAura* desktop_native_widget_aura) {
#if defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
  DesktopFactoryOzone* d_factory = DesktopFactoryOzone::GetInstance();

  return d_factory->CreateWindowTreeHost(native_widget_delegate,
                                         desktop_native_widget_aura);
#else
  NOTREACHED() << "Ozone builds should use DesktopWindowTreeHostMus codepath.";
  return nullptr;
#endif
}

}  // namespace views
