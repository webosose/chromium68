// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "ui/events/events_export.h"
#include "ui/events/platform/platform_event_source.h"

typedef struct _GPollFD GPollFD;
typedef struct _GSource GSource;

namespace ui {

class WaylandConnection;

class WaylandFdWatcherGlib {
 public:
  explicit WaylandFdWatcherGlib(WaylandConnection* connection);
  ~WaylandFdWatcherGlib();

 private:
  std::unique_ptr<GPollFD> wayland_poll_;

  GSource* glib_source_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(WaylandFdWatcherGlib);
};

}  // namespace ui
