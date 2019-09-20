// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/wayland_fd_watcher_glib.h"

#include <glib.h>
#include <wayland-client-core.h>

#include "ui/ozone/platform/wayland/wayland_connection.h"

namespace ui {

namespace {

struct GLibWaylandSource : public GSource {
  // Note: The GLibWaylandSource is created and destroyed by GLib. So its
  // constructor/destructor may or may not get called.
  WaylandConnection* connection;
  GPollFD* poll_fd;
};

gboolean WaylandSourcePrepare(GSource* source, gint* timeout_ms) {
  *timeout_ms = -1;
  return FALSE;
}

gboolean WaylandSourceCheck(GSource* source) {
  GLibWaylandSource* wayland_source = static_cast<GLibWaylandSource*>(source);
  return (wayland_source->poll_fd->revents & G_IO_IN) ? TRUE : FALSE;
}

gboolean WaylandSourceDispatch(GSource* source,
                               GSourceFunc unused_func,
                               gpointer data) {
  GLibWaylandSource* wayland_source = static_cast<GLibWaylandSource*>(source);
  wayland_source->connection->OnFileCanReadWithoutBlocking(-1);
  return TRUE;
}

GSourceFuncs g_wayland_source_funcs = {WaylandSourcePrepare, WaylandSourceCheck,
                                       WaylandSourceDispatch, nullptr};

}  // namespace

WaylandFdWatcherGlib::WaylandFdWatcherGlib(WaylandConnection* connection)
    : wayland_poll_(new GPollFD) {
  LOG(INFO) << "Starting WaylandFdWatcherGlib";

  wayland_poll_->fd = wl_display_get_fd(connection->display());
  wayland_poll_->events = G_IO_IN;
  wayland_poll_->revents = 0;

  GLibWaylandSource* wayland_source = static_cast<GLibWaylandSource*>(
      g_source_new(&g_wayland_source_funcs, sizeof(GLibWaylandSource)));

  wayland_source->connection = connection;
  wayland_source->poll_fd = wayland_poll_.get();

  glib_source_ = wayland_source;

  g_source_add_poll(glib_source_, wayland_poll_.get());
  g_source_set_can_recurse(glib_source_, TRUE);
  g_source_set_callback(glib_source_, nullptr, nullptr, nullptr);
  g_source_attach(glib_source_, g_main_context_default());
}

WaylandFdWatcherGlib::~WaylandFdWatcherGlib() {
  LOG(ERROR) << "Shutting down WaylandFdWatcherGlib";
  g_source_destroy(glib_source_);
  g_source_unref(glib_source_);
}

}  // namespace ui
