// Copyright 2014 Intel Corporation. All rights reserved
// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef OZONE_PLATFORM_OZONE_WAYLAND_WINDOW_H_
#define OZONE_PLATFORM_OZONE_WAYLAND_WINDOW_H_

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "ozone/platform/channel_observer.h"
#include "ozone/platform/input_content_type.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/events/platform/platform_event_dispatcher.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/platform_window/platform_window.h"
#include "ui/views/widget/desktop_aura/neva/ui_constants.h"

class SkBitmap;

namespace ui {

class BitmapCursorOzone;
class OzoneGpuPlatformSupportHost;
class PlatformWindowDelegate;
class WindowGroupConfiguration;
class WindowManagerWayland;

class OzoneWaylandWindow : public PlatformWindow,
                           public PlatformEventDispatcher,
                           public ChannelObserver {
 public:
  OzoneWaylandWindow(PlatformWindowDelegate* delegate,
                     OzoneGpuPlatformSupportHost* sender,
                     WindowManagerWayland* window_manager,
                     const gfx::Rect& bounds);
  ~OzoneWaylandWindow() override;

  unsigned GetHandle() const { return handle_; }
  PlatformWindowDelegate* GetDelegate() const { return delegate_; }

  // PlatformWindow:
  void InitPlatformWindow(PlatformWindowType type,
                          gfx::AcceleratedWidget parent_window) override;
  void SetTitle(const base::string16& title) override;
  void SetWindowShape(const SkPath& path) override;
  void SetOpacity(float opacity) override;
  void RequestDragData(const std::string& mime_type) override;
  void RequestSelectionData(const std::string& mime_type) override;
  void DragWillBeAccepted(uint32_t serial,
                          const std::string& mime_type) override;
  void DragWillBeRejected(uint32_t serial) override;
  gfx::Rect GetBounds() override;
  void SetBounds(const gfx::Rect& bounds) override;
  void Show() override;
  void Hide() override;
  void Close() override;
  void PrepareForShutdown() override;
  void SetCapture() override;
  void ReleaseCapture() override;
  bool HasCapture() const override;
  void ToggleFullscreen() override;
  void Maximize() override;
  void Minimize() override;
  void Restore() override;
  PlatformWindowState GetPlatformWindowState() const override;
  void SetCursor(PlatformCursor cursor) override;
  void MoveCursorTo(const gfx::Point& location) override;
  void ConfineCursorToBounds(const gfx::Rect& bounds) override;
  PlatformImeController* GetPlatformImeController() override;
  void SetRestoredBoundsInPixels(const gfx::Rect& bounds) override;
  gfx::Rect GetRestoredBoundsInPixels() const override;

  void SetWindowProperty(const std::string& name,
                         const std::string& value) override;
  void SetSurfaceId(int surface_id) override;
  void CreateGroup(const WindowGroupConfiguration&) override;
  void AttachToGroup(const std::string& group,
                     const std::string& layer) override;
  void FocusGroupOwner() override;
  void FocusGroupLayer() override;
  void DetachGroup() override;

  void ShowInputPanel() override;
  void HideInputPanel() override;
  void SetInputContentType(ui::TextInputType text_input_type, int text_input_flags) override;
  void SetSurroundingText(const std::string& text,
                          size_t cursor_position,
                          size_t anchor_position) override;
  void XInputActivate(const std::string& type) override;
  void XInputDeactivate() override;
  void XInputInvokeAction(uint32_t keysym,
                          ui::XInputKeySymbolType symbol_type,
                          ui::XInputEventType event_type) override;
  void SetCustomCursor(app_runtime::CustomCursorType type,
                       const std::string& path,
                       int hotspot_x,
                       int hotspot_y) override;
  void SetInputRegion(const std::vector<gfx::Rect>& region) override;
  void SetGroupKeyMask(ui::KeyMask key_mask) override;
  void SetKeyMask(ui::KeyMask key_mask, bool set) override;

  // PlatformEventDispatcher:
  bool CanDispatchEvent(const PlatformEvent& event) override;
  uint32_t DispatchEvent(const PlatformEvent& event) override;

  // ChannelObserver:
  void OnGpuProcessLaunched() override;
  void OnChannelDestroyed() override;

 private:
  void DeferredSendingToGpu();
  void SendWidgetState();
  void AddRegion();
  void ResetRegion();
  void SetCursor();
  void ValidateBounds();
  void SetCustomCursorFromBitmap(app_runtime::CustomCursorType type,
                                 SkBitmap* cursor_image,
                                 int hotspot_x,
                                 int hotspot_y);

  InputContentType InputContentTypeFromTextInputType(
      TextInputType text_input_type);
  PlatformWindowDelegate* delegate_;   // Not owned.
  OzoneGpuPlatformSupportHost* sender_;  // Not owned.
  WindowManagerWayland* window_manager_;  // Not owned.
  bool transparent_;
  gfx::Rect bounds_;
  unsigned handle_;
  unsigned parent_;
  ui::WidgetType type_;
  ui::WidgetState state_;
  SkRegion* region_;
  base::string16 title_;
  // The current cursor bitmap (immutable).
  scoped_refptr<BitmapCursorOzone> bitmap_;
  bool init_window_;
  base::WeakPtrFactory<OzoneWaylandWindow> weak_factory_;
  int surface_id_;

  DISALLOW_COPY_AND_ASSIGN(OzoneWaylandWindow);
};

}  // namespace ui

#endif  // OZONE_PLATFORM_OZONE_WAYLAND_WINDOW_H_
