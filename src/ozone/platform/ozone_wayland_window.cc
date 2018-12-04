// Copyright 2014 Intel Corporation. All rights reserved.
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

#include "ozone/platform/ozone_wayland_window.h"

#include <vector>
#include "base/bind.h"
#include "base/files/file.h"
#include "base/memory/ref_counted_memory.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread_restrictions.h"
#include "ozone/platform/messages.h"
#include "ozone/platform/ozone_gpu_platform_support_host.h"
#include "ozone/platform/window_manager_wayland.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/cursor/ozone/bitmap_cursor_factory_ozone.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/ozone/events_ozone.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/ozone/public/cursor_factory_ozone.h"
#include "ui/platform_window/neva/window_group_configuration.h"
#include "ui/platform_window/platform_window_delegate.h"

namespace ui {

namespace {

scoped_refptr<base::RefCountedBytes> ReadFileData(const base::FilePath& path) {
  if (path.empty())
    return 0;

  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid())
    return 0;

  int64_t length = file.GetLength();
  if (length > 0 && length < INT_MAX) {
    int size = static_cast<int>(length);
    std::vector<unsigned char> raw_data;
    raw_data.resize(size);
    char* data = reinterpret_cast<char*>(&(raw_data.front()));
    if (file.ReadAtCurrentPos(data, size) == length)
      return base::RefCountedBytes::TakeVector(&raw_data);
  }
  return 0;
}

void CreateBitmapFromPng(
    app_runtime::CustomCursorType type,
    const std::string& path,
    int hotspot_x,
    int hotspot_y,
    const base::Callback<void(app_runtime::CustomCursorType,
                              SkBitmap*,
                              int,
                              int)> callback) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  scoped_refptr<base::RefCountedBytes> memory(
      ReadFileData(base::FilePath(path)));
  if (!memory.get()) {
    LOG(INFO) << "Unable to read file path = " << path;
    return;
  }

  SkBitmap* bitmap = new SkBitmap();
  if (!gfx::PNGCodec::Decode(memory->front(), memory->size(), bitmap)) {
    LOG(INFO) << "Unable to decode image path = " << path;
    delete bitmap;
    return;
  }

  base::MessageLoopForUI::current()->task_runner()->PostTask(
      FROM_HERE, base::Bind(callback, type, bitmap, hotspot_x, hotspot_y));
}

}  // namespace

OzoneWaylandWindow::OzoneWaylandWindow(PlatformWindowDelegate* delegate,
                                       OzoneGpuPlatformSupportHost* sender,
                                       WindowManagerWayland* window_manager,
                                       const gfx::Rect& bounds)
    : delegate_(delegate),
      sender_(sender),
      window_manager_(window_manager),
      transparent_(false),
      bounds_(bounds),
      parent_(0),
      type_(WidgetType::WINDOWFRAMELESS),
      state_(WidgetState::UNINITIALIZED),
      region_(NULL),
      init_window_(false),
      weak_factory_(this) {
  static int opaque_handle = 0;
  opaque_handle++;
  handle_ = opaque_handle;
  delegate_->OnAcceleratedWidgetAvailable(opaque_handle);

  char* env;
  if ((env = getenv("OZONE_WAYLAND_IVI_SURFACE_ID")))
    surface_id_ = atoi(env);
  else
    surface_id_ = getpid();
  PlatformEventSource::GetInstance()->AddPlatformEventDispatcher(this);
  sender_->AddChannelObserver(this);
  window_manager_->OnRootWindowCreated(this);
}

OzoneWaylandWindow::~OzoneWaylandWindow() {
  sender_->RemoveChannelObserver(this);
  PlatformEventSource::GetInstance()->RemovePlatformEventDispatcher(this);
  sender_->Send(new WaylandDisplay_DestroyWindow(handle_));
  if (region_)
    delete region_;
}

void OzoneWaylandWindow::InitPlatformWindow(
    PlatformWindowType type, gfx::AcceleratedWidget parent_window) {
  switch (type) {
    case PLATFORM_WINDOW_TYPE_POPUP:
    case PLATFORM_WINDOW_TYPE_MENU: {
      parent_ = parent_window;
      if (!parent_ && window_manager_->GetActiveWindow())
        parent_ = window_manager_->GetActiveWindow()->GetHandle();
      type_ = ui::WidgetType::POPUP;
      ValidateBounds();
      break;
    }
    case PLATFORM_WINDOW_TYPE_TOOLTIP: {
      parent_ = parent_window;
      if (!parent_ && window_manager_->GetActiveWindow())
        parent_ = window_manager_->GetActiveWindow()->GetHandle();
      type_ = ui::WidgetType::TOOLTIP;
      bounds_.set_origin(gfx::Point(0, 0));
      break;
    }
    case PLATFORM_WINDOW_TYPE_BUBBLE:
    case PLATFORM_WINDOW_TYPE_WINDOW:
      parent_ = 0;
      type_ = ui::WidgetType::WINDOW;
      break;
    case PLATFORM_WINDOW_TYPE_WINDOW_FRAMELESS:
      NOTIMPLEMENTED();
      break;
    default:
      break;
  }

  init_window_ = true;

  if (!sender_->IsConnected())
    return;

  sender_->Send(new WaylandDisplay_InitWindow(handle_,
                                              parent_,
                                              bounds_.x(),
                                              bounds_.y(),
                                              type_,
                                              surface_id_));
}

void OzoneWaylandWindow::SetTitle(const base::string16& title) {
  title_ = title;
  if (!sender_->IsConnected())
    return;

  sender_->Send(new WaylandDisplay_Title(handle_, title_));
}

void OzoneWaylandWindow::SetSurfaceId(int surface_id) {
  surface_id_ = surface_id;
}

void OzoneWaylandWindow::SetWindowShape(const SkPath& path) {
  ResetRegion();
  if (transparent_)
    return;

  region_ = new SkRegion();
  SkRegion clip_region;
  clip_region.setRect(0, 0, bounds_.width(), bounds_.height());
  region_->setPath(path, clip_region);
  AddRegion();
}

void OzoneWaylandWindow::SetOpacity(float opacity) {
  if (opacity == 1.f) {
    if (transparent_) {
      AddRegion();
      transparent_ = false;
    }
  } else if (!transparent_) {
    ResetRegion();
    transparent_ = true;
  }
}

void OzoneWaylandWindow::RequestDragData(const std::string& mime_type) {
  sender_->Send(new WaylandDisplay_RequestDragData(mime_type));
}

void OzoneWaylandWindow::RequestSelectionData(const std::string& mime_type) {
  sender_->Send(new WaylandDisplay_RequestSelectionData(mime_type));
}

void OzoneWaylandWindow::DragWillBeAccepted(uint32_t serial,
                                            const std::string& mime_type) {
  sender_->Send(new WaylandDisplay_DragWillBeAccepted(serial, mime_type));
}

void OzoneWaylandWindow::DragWillBeRejected(uint32_t serial) {
  sender_->Send(new WaylandDisplay_DragWillBeRejected(serial));
}

gfx::Rect OzoneWaylandWindow::GetBounds() {
  return bounds_;
}

void OzoneWaylandWindow::SetBounds(const gfx::Rect& bounds) {
  int original_x = bounds_.x();
  int original_y = bounds_.y();
  bounds_ = bounds;
  if (type_ == ui::WidgetType::TOOLTIP)
    ValidateBounds();

  if ((original_x != bounds_.x()) || (original_y  != bounds_.y())) {
    sender_->Send(new WaylandDisplay_MoveWindow(handle_, parent_,
                                                type_, bounds_));
  }

  delegate_->OnBoundsChanged(bounds_);
}

void OzoneWaylandWindow::Show() {
  state_ = WidgetState::SHOW;
  SendWidgetState();
}

void OzoneWaylandWindow::Hide() {
  state_ = WidgetState::HIDE;

  if (type_ == ui::WidgetType::TOOLTIP)
    delegate_->OnCloseRequest();
  else
    SendWidgetState();
}

void OzoneWaylandWindow::Close() {
  window_manager_->OnRootWindowClosed(this);
}

void OzoneWaylandWindow::PrepareForShutdown() {}

void OzoneWaylandWindow::SetCapture() {
  window_manager_->GrabEvents(handle_);
}

void OzoneWaylandWindow::ReleaseCapture() {
  window_manager_->UngrabEvents(handle_);
}

bool OzoneWaylandWindow::HasCapture() const {
  return false;
}

void OzoneWaylandWindow::ToggleFullscreen() {
  display::Screen *screen = display::Screen::GetScreen();
  if (!screen)
    NOTREACHED() << "Unable to retrieve valid display::Screen";

  SetBounds(screen->GetPrimaryDisplay().bounds());
  state_ = WidgetState::FULLSCREEN;
  SendWidgetState();
}

void OzoneWaylandWindow::Maximize() {
  display::Screen *screen = display::Screen::GetScreen();
  if (!screen)
    NOTREACHED() << "Unable to retrieve valid display::Screen";
  SetBounds(screen->GetPrimaryDisplay().bounds());
  state_ = WidgetState::MAXIMIZED;
  SendWidgetState();
}

void OzoneWaylandWindow::Minimize() {
  SetBounds(gfx::Rect());
  state_ = WidgetState::MINIMIZED;
  SendWidgetState();
}

void OzoneWaylandWindow::Restore() {
  window_manager_->Restore(this);
  state_ = WidgetState::RESTORE;
  SendWidgetState();
}

PlatformWindowState OzoneWaylandWindow::GetPlatformWindowState() const {
  NOTIMPLEMENTED();
    return PLATFORM_WINDOW_STATE_UNKNOWN;
}

void OzoneWaylandWindow::SetCursor(PlatformCursor cursor) {
  if (window_manager_->GetPlatformCursor() == cursor)
    return;

  scoped_refptr<BitmapCursorOzone> bitmap =
      BitmapCursorFactoryOzone::GetBitmapCursor(cursor);
  bitmap_ = bitmap;
  window_manager_->SetPlatformCursor(cursor);
  if (!sender_->IsConnected())
    return;

  SetCursor();
}

void OzoneWaylandWindow::MoveCursorTo(const gfx::Point& location) {
  sender_->Send(new WaylandDisplay_MoveCursor(location));
}

void OzoneWaylandWindow::ConfineCursorToBounds(const gfx::Rect& bounds) {
}

////////////////////////////////////////////////////////////////////////////////
// WindowTreeHostDelegateWayland, ui::PlatformEventDispatcher implementation:
bool OzoneWaylandWindow::CanDispatchEvent(
    const ui::PlatformEvent& ne) {
  return window_manager_->event_grabber() == gfx::AcceleratedWidget(handle_);
}

uint32_t OzoneWaylandWindow::DispatchEvent(
    const ui::PlatformEvent& ne) {
  DispatchEventFromNativeUiEvent(
      ne, base::Bind(&PlatformWindowDelegate::DispatchEvent,
                     base::Unretained(delegate_)));
  return POST_DISPATCH_STOP_PROPAGATION;
}

void OzoneWaylandWindow::OnGpuProcessLaunched() {
  if (sender_->IsConnected())
    DeferredSendingToGpu();
}

void OzoneWaylandWindow::DeferredSendingToGpu() {
  sender_->Send(new WaylandDisplay_Create(handle_));
  if (init_window_)
    sender_->Send(new WaylandDisplay_InitWindow(handle_,
                                                parent_,
                                                bounds_.x(),
                                                bounds_.y(),
                                                type_,
                                                surface_id_));

  if (state_ != WidgetState::UNINITIALIZED)
    sender_->Send(new WaylandDisplay_State(handle_, state_));

  if (title_.length())
    sender_->Send(new WaylandDisplay_Title(handle_, title_));

  AddRegion();
  if (bitmap_)
    SetCursor();
}

void OzoneWaylandWindow::OnChannelDestroyed() {
}

void OzoneWaylandWindow::SendWidgetState() {
  if (!sender_->IsConnected())
    return;

  sender_->Send(new WaylandDisplay_State(handle_, state_));
}

void OzoneWaylandWindow::AddRegion() {
  if (sender_->IsConnected() && region_ && !region_->isEmpty()) {
     const SkIRect& rect = region_->getBounds();
     sender_->Send(new WaylandDisplay_AddRegion(handle_,
                                                rect.left(),
                                                rect.top(),
                                                rect.right(),
                                                rect.bottom()));
  }
}

void OzoneWaylandWindow::ResetRegion() {
  if (region_) {
    if (sender_->IsConnected() && !region_->isEmpty()) {
      const SkIRect& rect = region_->getBounds();
      sender_->Send(new WaylandDisplay_SubRegion(handle_,
                                                 rect.left(),
                                                 rect.top(),
                                                 rect.right(),
                                                 rect.bottom()));
    }

    delete region_;
    region_ = NULL;
  }
}

void OzoneWaylandWindow::SetCursor() {
  if (bitmap_) {
    sender_->Send(new WaylandDisplay_CursorSet(bitmap_->bitmaps(),
                                               bitmap_->hotspot()));
  } else {
    sender_->Send(new WaylandDisplay_CursorSet(std::vector<SkBitmap>(),
                                               gfx::Point()));
  }
}

void OzoneWaylandWindow::ValidateBounds() {
  DCHECK(parent_);
  if (!parent_) {
    LOG(INFO) << "Validate bounds will not do, parent is null";
    return;
  }

  gfx::Rect parent_bounds = window_manager_->GetWindow(parent_)->GetBounds();
  int x = bounds_.x() - parent_bounds.x();
  int y = bounds_.y() - parent_bounds.y();

  if (x < parent_bounds.x()) {
    x = parent_bounds.x();
  } else {
    int width = x + bounds_.width();
    if (width > parent_bounds.width())
      x -= width - parent_bounds.width();
  }

  if (y < parent_bounds.y()) {
    y = parent_bounds.y();
  } else {
    int height = y + bounds_.height();
    if (height > parent_bounds.height())
      y -= height - parent_bounds.height();
  }

  bounds_.set_origin(gfx::Point(x, y));
}

PlatformImeController* OzoneWaylandWindow::GetPlatformImeController() {
  return nullptr;
}

void OzoneWaylandWindow::SetWindowProperty(const std::string& name,
                                           const std::string& value) {
  sender_->Send(new WaylandDisplay_SetWindowProperty(handle_, name, value));
}

void OzoneWaylandWindow::CreateGroup(
    const ui::WindowGroupConfiguration& config) {
  sender_->Send(new WaylandDisplay_CreateWindowGroup(handle_, config));
}

void OzoneWaylandWindow::AttachToGroup(const std::string& group,
                                       const std::string& layer) {
  sender_->Send(new WaylandDisplay_AttachToWindowGroup(handle_, group, layer));
}

void OzoneWaylandWindow::FocusGroupOwner() {
  sender_->Send(new WaylandDisplay_FocusWindowGroupOwner(handle_));
}

void OzoneWaylandWindow::FocusGroupLayer() {
  sender_->Send(new WaylandDisplay_FocusWindowGroupLayer(handle_));
}

void OzoneWaylandWindow::DetachGroup() {
  sender_->Send(new WaylandDisplay_DetachWindowGroup(handle_));
}

void OzoneWaylandWindow::ShowInputPanel() {
  sender_->Send(new WaylandDisplay_ShowInputPanel(handle_));
}

void OzoneWaylandWindow::HideInputPanel() {
  sender_->Send(new WaylandDisplay_HideInputPanel());
}

void OzoneWaylandWindow::SetInputContentType(ui::TextInputType text_input_type,
                                             int text_input_flags) {
  sender_->Send(new WaylandDisplay_SetInputContentType(
      InputContentTypeFromTextInputType(text_input_type), text_input_flags, handle_));
}

void OzoneWaylandWindow::SetSurroundingText(const std::string& text,
                                            size_t cursor_position,
                                            size_t anchor_position) {
  sender_->Send(new WaylandDisplay_SetSurroundingText(text, cursor_position,
                                                      anchor_position));
}

InputContentType OzoneWaylandWindow::InputContentTypeFromTextInputType(
    TextInputType text_input_type) {
  switch (text_input_type) {
    case ui::TEXT_INPUT_TYPE_NONE:
      return INPUT_CONTENT_TYPE_NONE;
    case ui::TEXT_INPUT_TYPE_TEXT:
      return INPUT_CONTENT_TYPE_TEXT;
    case ui::TEXT_INPUT_TYPE_PASSWORD:
      return INPUT_CONTENT_TYPE_PASSWORD;
    case ui::TEXT_INPUT_TYPE_SEARCH:
      return INPUT_CONTENT_TYPE_SEARCH;
    case ui::TEXT_INPUT_TYPE_EMAIL:
      return INPUT_CONTENT_TYPE_EMAIL;
    case ui::TEXT_INPUT_TYPE_NUMBER:
      return INPUT_CONTENT_TYPE_NUMBER;
    case ui::TEXT_INPUT_TYPE_TELEPHONE:
      return INPUT_CONTENT_TYPE_TELEPHONE;
    case ui::TEXT_INPUT_TYPE_URL:
      return INPUT_CONTENT_TYPE_URL;
    case ui::TEXT_INPUT_TYPE_DATE:
      return INPUT_CONTENT_TYPE_DATE;
    case ui::TEXT_INPUT_TYPE_DATE_TIME:
      return INPUT_CONTENT_TYPE_DATE_TIME;
    case ui::TEXT_INPUT_TYPE_DATE_TIME_LOCAL:
      return INPUT_CONTENT_TYPE_DATE_TIME_LOCAL;
    case ui::TEXT_INPUT_TYPE_MONTH:
      return INPUT_CONTENT_TYPE_MONTH;
    case ui::TEXT_INPUT_TYPE_TIME:
      return INPUT_CONTENT_TYPE_TIME;
    case ui::TEXT_INPUT_TYPE_WEEK:
      return INPUT_CONTENT_TYPE_WEEK;
    case ui::TEXT_INPUT_TYPE_TEXT_AREA:
      return INPUT_CONTENT_TYPE_TEXT_AREA;
    case ui::TEXT_INPUT_TYPE_CONTENT_EDITABLE:
      return INPUT_CONTENT_TYPE_CONTENT_EDITABLE;
    case ui::TEXT_INPUT_TYPE_DATE_TIME_FIELD:
      return INPUT_CONTENT_TYPE_DATE_TIME_FIELD;
    default:
      return INPUT_CONTENT_TYPE_TEXT;
  }
}

void OzoneWaylandWindow::XInputActivate(const std::string& type) {
  sender_->Send(new WaylandDisplay_XInputActivate(type));
}

void OzoneWaylandWindow::XInputDeactivate() {
  sender_->Send(new WaylandDisplay_XInputDeactivate());
}

void OzoneWaylandWindow::XInputInvokeAction(uint32_t keysym,
                                            ui::XInputKeySymbolType symbol_type,
                                            ui::XInputEventType event_type) {
  sender_->Send(
      new WaylandDisplay_XInputInvokeAction(keysym, symbol_type, event_type));
}

void OzoneWaylandWindow::SetCustomCursor(app_runtime::CustomCursorType type,
                                         const std::string& path,
                                         int hotspot_x,
                                         int hotspot_y) {
  if (type == app_runtime::CustomCursorType::kPath) {
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&CreateBitmapFromPng, type, path, hotspot_x,
                   hotspot_y,
                   base::Bind(&OzoneWaylandWindow::SetCustomCursorFromBitmap,
                              weak_factory_.GetWeakPtr())));
  } else if (type == app_runtime::CustomCursorType::kBlank) {
    // BLANK : Disable cursor(hiding cursor)
    sender_->Send(new WaylandDisplay_CursorSet(std::vector<SkBitmap>(),
                                               gfx::Point(254, 254)));
    bitmap_ = nullptr;
  } else {
    // NOT_USE : Restore cursor(wayland cursor or IM's cursor)
    sender_->Send(new WaylandDisplay_CursorSet(std::vector<SkBitmap>(),
                                               gfx::Point(255, 255)));
    bitmap_ = nullptr;
  }
}

void OzoneWaylandWindow::SetCustomCursorFromBitmap(app_runtime::CustomCursorType type,
                                                   SkBitmap* cursor_image,
                                                   int hotspot_x,
                                                   int hotspot_y) {
  if (!cursor_image) {
    SetCustomCursor(app_runtime::CustomCursorType::kNotUse, "", 0, 0);
    return;
  }

  SetCursor(CursorFactoryOzone::GetInstance()->CreateImageCursor(
    *cursor_image, gfx::Point(hotspot_x, hotspot_y), 0));
  delete cursor_image;
}

void OzoneWaylandWindow::SetInputRegion(const std::vector<gfx::Rect>& region) {
  sender_->Send(new WaylandDisplay_SetInputRegion(handle_, region));
}

void OzoneWaylandWindow::SetGroupKeyMask(ui::KeyMask key_mask) {
  sender_->Send(new WaylandDisplay_SetGroupKeyMask(handle_, key_mask));
}

void OzoneWaylandWindow::SetKeyMask(ui::KeyMask key_mask, bool set) {
  sender_->Send(new WaylandDisplay_SetKeyMask(handle_, key_mask, set));
}

}  // namespace ui
