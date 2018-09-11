// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/desktop_aura/desktop_factory_ozone.h"

#include "base/logging.h"

// Added for external ozone wayland port
#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
#include <memory>
#include "ui/ozone/platform_object.h"
#endif

namespace views {

// static
DesktopFactoryOzone* DesktopFactoryOzone::impl_ = nullptr;

DesktopFactoryOzone::DesktopFactoryOzone() {
// Added for external ozone wayland port
#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
  DCHECK(!impl_) << "There should only be a single DesktopFactoryOzone.";
  impl_ = this;
#endif
}

DesktopFactoryOzone::~DesktopFactoryOzone() {
// Added for external ozone wayland port
#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
  DCHECK_EQ(impl_, this);
  impl_ = nullptr;
#endif
}

DesktopFactoryOzone* DesktopFactoryOzone::GetInstance() {
// Added for external ozone wayland port
#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
  if (!impl_) {
    std::unique_ptr<DesktopFactoryOzone> factory =
        ui::PlatformObject<DesktopFactoryOzone>::Create();

    // TODO(tonikitoo): Currently need to leak this object.
    DesktopFactoryOzone* leaky = factory.release();
    DCHECK_EQ(impl_, leaky);
  }
#else
  CHECK(impl_) << "DesktopFactoryOzone accessed before constructed";
#endif
  return impl_;
}

// Disabled for external ozone wayland port
#if !(defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL))
void DesktopFactoryOzone::SetInstance(DesktopFactoryOzone* impl) {
  impl_ = impl;
}
#endif

} // namespace views
