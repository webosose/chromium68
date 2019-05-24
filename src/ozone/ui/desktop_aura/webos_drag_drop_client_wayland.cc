// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/ui/desktop_aura/webos_drag_drop_client_wayland.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ozone/ui/desktop_aura/drag_drop_tracker.h"
#include "ozone/ui/desktop_aura/drag_image_view.h"
#include "ui/aura/client/capture_client.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/client/drag_drop_client_observer.h"
#include "ui/aura/client/drag_drop_delegate.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/base/hit_test.h"
#include "ui/events/event.h"
#include "ui/events/event_utils.h"
#include "ui/gfx/animation/linear_animation.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/path.h"
#include "ui/views/widget/native_widget_aura.h"
#include "ui/wm/core/coordinate_conversion.h"

namespace views {
namespace {

// The duration of the drag cancel animation in millisecond.
constexpr base::TimeDelta kCancelAnimationDuration =
    base::TimeDelta::FromMilliseconds(250);
// The frame rate of the drag cancel animation in hertz.
const int kCancelAnimationFrameRate = 60;

// Adjusts the drag image bounds such that the new bounds are scaled by |scale|
// and translated by the |drag_image_offset| and and additional
// |vertical_offset|.
gfx::Rect AdjustDragImageBoundsForScaleAndOffset(
    const gfx::Rect& drag_image_bounds,
    int vertical_offset,
    float scale,
    gfx::Vector2d* drag_image_offset) {
  gfx::Point final_origin = drag_image_bounds.origin();
  gfx::SizeF final_size = gfx::SizeF(drag_image_bounds.size());
  final_size.Scale(scale);
  drag_image_offset->set_x(drag_image_offset->x() * scale);
  drag_image_offset->set_y(drag_image_offset->y() * scale);
  int total_x_offset = drag_image_offset->x();
  int total_y_offset = drag_image_offset->y() - vertical_offset;
  final_origin.Offset(-total_x_offset, -total_y_offset);
  return gfx::ToEnclosingRect(
      gfx::RectF(gfx::PointF(final_origin), final_size));
}
}  // namespace

class DragDropTrackerDelegate : public aura::WindowDelegate {
 public:
  explicit DragDropTrackerDelegate(WebosDragDropClientWayland* controller)
      : drag_drop_controller_(controller) {}
  ~DragDropTrackerDelegate() override = default;

  // Overridden from WindowDelegate:
  gfx::Size GetMinimumSize() const override { return gfx::Size(); }

  gfx::Size GetMaximumSize() const override { return gfx::Size(); }

  void OnBoundsChanged(const gfx::Rect& old_bounds,
                       const gfx::Rect& new_bounds) override {}
  gfx::NativeCursor GetCursor(const gfx::Point& point) override {
    return gfx::kNullCursor;
  }
  int GetNonClientComponent(const gfx::Point& point) const override {
    return HTCAPTION;
  }
  bool ShouldDescendIntoChildForEventHandling(
      aura::Window* child,
      const gfx::Point& location) override {
    return true;
  }
  bool CanFocus() override { return true; }
  void OnCaptureLost() override {
    if (drag_drop_controller_->IsDragDropInProgress())
      drag_drop_controller_->DragCancel();
  }
  void OnPaint(const ui::PaintContext& context) override {}
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override {}
  void OnWindowDestroying(aura::Window* window) override {}
  void OnWindowDestroyed(aura::Window* window) override {}
  void OnWindowTargetVisibilityChanged(bool visible) override {}
  bool HasHitTestMask() const override { return true; }
  void GetHitTestMask(gfx::Path* mask) const override {
    DCHECK(mask->isEmpty());
  }

 private:
  WebosDragDropClientWayland* drag_drop_controller_;

  DISALLOW_COPY_AND_ASSIGN(DragDropTrackerDelegate);
};

////////////////////////////////////////////////////////////////////////////////
// WebosDragDropClientWayland, public:

WebosDragDropClientWayland::WebosDragDropClientWayland(aura::Window* window)
    : drag_data_(NULL),
      drag_operation_(0),
      root_window_(window),
      drag_window_(NULL),
      drag_source_window_(NULL),
      should_block_during_drag_drop_(true),
      drag_drop_window_delegate_(new DragDropTrackerDelegate(this)),
      current_drag_event_source_(ui::DragDropTypes::DRAG_EVENT_SOURCE_MOUSE),
      weak_factory_(this) {
  if (root_window_)
    root_window_->AddPreTargetHandler(this);
}

WebosDragDropClientWayland::~WebosDragDropClientWayland() {
  if (root_window_)
    root_window_->RemovePreTargetHandler(this);
  Cleanup();
  if (cancel_animation_)
    cancel_animation_->End();
  if (drag_image_)
    drag_image_.reset();
}

int WebosDragDropClientWayland::StartDragAndDrop(
    const ui::OSExchangeData& data,
    aura::Window* root_window,
    aura::Window* source_window,
    const gfx::Point& screen_location,
    int operation,
    ui::DragDropTypes::DragEventSource source) {
  if (IsDragDropInProgress())
    return 0;

  const ui::OSExchangeData::Provider* provider = &data.provider();

  UMA_HISTOGRAM_ENUMERATION("Event.DragDrop.Start", source,
                            ui::DragDropTypes::DRAG_EVENT_SOURCE_COUNT);

  current_drag_event_source_ = source;
  DragDropTracker* tracker =
      new DragDropTracker(root_window, drag_drop_window_delegate_.get());
  tracker->TakeCapture();
  drag_drop_tracker_.reset(tracker);
  drag_source_window_ = source_window;
  if (drag_source_window_)
    drag_source_window_->AddObserver(this);

  drag_data_ = &data;
  drag_operation_ = operation;

  float drag_image_scale = 1;
  int drag_image_vertical_offset = 0;
  gfx::Point start_location = screen_location;
  drag_image_final_bounds_for_cancel_animation_ =
      gfx::Rect(start_location - provider->GetDragImageOffset(),
                provider->GetDragImage().size());
  drag_image_ =
      std::make_unique<DragImageView>(source_window->GetRootWindow(), source);
  drag_image_->SetImage(provider->GetDragImage());
  drag_image_offset_ = provider->GetDragImageOffset();
  gfx::Rect drag_image_bounds(start_location, drag_image_->GetPreferredSize());
  drag_image_bounds = AdjustDragImageBoundsForScaleAndOffset(
      drag_image_bounds, drag_image_vertical_offset, drag_image_scale,
      &drag_image_offset_);
  drag_image_->SetBoundsInScreen(drag_image_bounds);
  drag_image_->SetWidgetVisible(true);

  drag_window_ = NULL;

  // Ends cancel animation if it's in progress.
  if (cancel_animation_)
    cancel_animation_->End();

  for (aura::client::DragDropClientObserver& observer : observers_)
    observer.OnDragStarted();

  if (should_block_during_drag_drop_) {
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    quit_closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

  if (drag_operation_ == 0) {
    UMA_HISTOGRAM_ENUMERATION("Event.DragDrop.Cancel", source,
                              ui::DragDropTypes::DRAG_EVENT_SOURCE_COUNT);
  } else {
    UMA_HISTOGRAM_ENUMERATION("Event.DragDrop.Drop", source,
                              ui::DragDropTypes::DRAG_EVENT_SOURCE_COUNT);
  }

  if (!cancel_animation_.get() || !cancel_animation_->is_animating()) {
    // If drag cancel animation is running, this cleanup is done when the
    // animation completes.
    if (drag_source_window_)
      drag_source_window_->RemoveObserver(this);
    drag_source_window_ = NULL;
  }

  return drag_operation_;
}

void WebosDragDropClientWayland::DragCancel() {
  DoDragCancel(kCancelAnimationDuration);
}

bool WebosDragDropClientWayland::IsDragDropInProgress() {
  return !!drag_drop_tracker_.get();
}

void WebosDragDropClientWayland::AddObserver(
    aura::client::DragDropClientObserver* observer) {
  observers_.AddObserver(observer);
}

void WebosDragDropClientWayland::RemoveObserver(
    aura::client::DragDropClientObserver* observer) {
  observers_.RemoveObserver(observer);
}

void WebosDragDropClientWayland::OnKeyEvent(ui::KeyEvent* event) {
  if (IsDragDropInProgress() && event->key_code() == ui::VKEY_ESCAPE) {
    DragCancel();
    event->StopPropagation();
  }
}

void WebosDragDropClientWayland::OnMouseEvent(ui::MouseEvent* event) {
  if (!IsDragDropInProgress())
    return;

  // If current drag session was not started by mouse, dont process this mouse
  // event, but consume it so it does not interfere with current drag session.
  if (current_drag_event_source_ !=
      ui::DragDropTypes::DRAG_EVENT_SOURCE_MOUSE) {
    event->StopPropagation();
    return;
  }

  aura::Window* translated_target = drag_drop_tracker_->GetTarget(*event);
  if (!translated_target) {
    DragCancel();
    event->StopPropagation();
    return;
  }
  std::unique_ptr<ui::LocatedEvent> translated_event(
      drag_drop_tracker_->ConvertEvent(translated_target, *event));
  switch (translated_event->type()) {
    case ui::ET_MOUSE_DRAGGED:
      DragUpdate(translated_target, *translated_event.get());
      break;
    case ui::ET_MOUSE_RELEASED:
      Drop(translated_target, *translated_event.get());
      break;
    default:
      // We could also reach here because RootWindow may sometimes generate a
      // bunch of fake mouse events
      // (aura::RootWindow::PostMouseMoveEventAfterWindowChange).
      break;
  }
  event->StopPropagation();
}

void WebosDragDropClientWayland::OnWindowDestroyed(aura::Window* window) {
  if (drag_window_ == window)
    drag_window_ = NULL;
  if (drag_source_window_ == window)
    drag_source_window_ = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// WebosDragDropClientWayland, protected:

gfx::LinearAnimation* WebosDragDropClientWayland::CreateCancelAnimation(
    base::TimeDelta duration,
    int frame_rate,
    gfx::AnimationDelegate* delegate) {
  return new gfx::LinearAnimation(duration, frame_rate, delegate);
}

void WebosDragDropClientWayland::DragUpdate(aura::Window* target,
                                            const ui::LocatedEvent& event) {
  int op = ui::DragDropTypes::DRAG_NONE;
  if (target != drag_window_) {
    if (drag_window_) {
      aura::client::DragDropDelegate* delegate =
          aura::client::GetDragDropDelegate(drag_window_);
      if (delegate)
        delegate->OnDragExited();
      if (drag_window_ != drag_source_window_)
        drag_window_->RemoveObserver(this);
    }
    drag_window_ = target;
    // We are already an observer of |drag_source_window_| so no need to add.
    if (drag_window_ != drag_source_window_)
      drag_window_->AddObserver(this);
    aura::client::DragDropDelegate* delegate =
        aura::client::GetDragDropDelegate(drag_window_);
    if (delegate) {
      ui::DropTargetEvent e(*drag_data_, gfx::Point(), gfx::Point(),
                            drag_operation_);
      e.set_location_f(event.location_f());
      e.set_root_location_f(event.root_location_f());
      e.set_flags(event.flags());
      ui::Event::DispatcherApi(&e).set_target(target);
      delegate->OnDragEntered(e);
    }
  } else {
    aura::client::DragDropDelegate* delegate =
        aura::client::GetDragDropDelegate(drag_window_);
    if (delegate) {
      ui::DropTargetEvent e(*drag_data_, gfx::Point(), gfx::Point(),
                            drag_operation_);
      e.set_location_f(event.location_f());
      e.set_root_location_f(event.root_location_f());
      e.set_flags(event.flags());
      ui::Event::DispatcherApi(&e).set_target(target);
      op = delegate->OnDragUpdated(e);
      gfx::NativeCursor cursor = ui::CursorType::kNoDrop;
      if (op & ui::DragDropTypes::DRAG_COPY)
        cursor = ui::CursorType::kCopy;
      else if (op & ui::DragDropTypes::DRAG_LINK)
        cursor = ui::CursorType::kAlias;
      else if (op & ui::DragDropTypes::DRAG_MOVE)
        cursor = ui::CursorType::kGrabbing;
      SetCursor(cursor);
    }
  }

  DCHECK(drag_image_.get());
  if (drag_image_->visible()) {
    gfx::Point root_location_in_screen = event.root_location();
    ::wm::ConvertPointToScreen(target->GetRootWindow(),
                               &root_location_in_screen);
    drag_image_->SetScreenPosition(root_location_in_screen -
                                   drag_image_offset_);
  }
}

void WebosDragDropClientWayland::Drop(aura::Window* target,
                                      const ui::LocatedEvent& event) {
  SetCursor(ui::CursorType::kPointer);

  // We must guarantee that a target gets a OnDragEntered before Drop. WebKit
  // depends on not getting a Drop without DragEnter. This behavior is
  // consistent with drag/drop on other platforms.
  if (target != drag_window_)
    DragUpdate(target, event);
  DCHECK(target == drag_window_);

  aura::client::DragDropDelegate* delegate =
      aura::client::GetDragDropDelegate(target);
  if (delegate) {
    ui::DropTargetEvent e(*drag_data_, gfx::Point(), gfx::Point(),
                          drag_operation_);
    e.set_location_f(event.location_f());
    e.set_root_location_f(event.root_location_f());
    e.set_flags(event.flags());
    ui::Event::DispatcherApi(&e).set_target(target);
    drag_operation_ = delegate->OnPerformDrop(e);
    if (drag_operation_ == 0)
      StartCanceledAnimation(kCancelAnimationDuration);
    else
      drag_image_.reset();
  } else {
    drag_image_.reset();
  }

  Cleanup();
  if (should_block_during_drag_drop_)
    quit_closure_.Run();
}

////////////////////////////////////////////////////////////////////////////////
// WebosDragDropClientWayland, private:

void WebosDragDropClientWayland::AnimationEnded(
    const gfx::Animation* animation) {
  cancel_animation_.reset();

  // By the time we finish animation, another drag/drop session may have
  // started. We do not want to destroy the drag image in that case.
  if (!IsDragDropInProgress())
    drag_image_.reset();
}

void WebosDragDropClientWayland::DoDragCancel(
    base::TimeDelta drag_cancel_animation_duration) {
  SetCursor(ui::CursorType::kPointer);

  // |drag_window_| can be NULL if we have just started the drag and have not
  // received any DragUpdates, or, if the |drag_window_| gets destroyed during
  // a drag/drop.
  aura::client::DragDropDelegate* delegate =
      drag_window_ ? aura::client::GetDragDropDelegate(drag_window_) : NULL;
  if (delegate)
    delegate->OnDragExited();

  Cleanup();
  drag_operation_ = 0;
  StartCanceledAnimation(drag_cancel_animation_duration);
  if (should_block_during_drag_drop_)
    quit_closure_.Run();
}

void WebosDragDropClientWayland::AnimationProgressed(
    const gfx::Animation* animation) {
  gfx::Rect current_bounds = animation->CurrentValueBetween(
      drag_image_initial_bounds_for_cancel_animation_,
      drag_image_final_bounds_for_cancel_animation_);
  drag_image_->SetBoundsInScreen(current_bounds);
}

void WebosDragDropClientWayland::AnimationCanceled(
    const gfx::Animation* animation) {
  AnimationEnded(animation);
}

void WebosDragDropClientWayland::StartCanceledAnimation(
    base::TimeDelta animation_duration) {
  DCHECK(drag_image_.get());
  drag_image_initial_bounds_for_cancel_animation_ =
      drag_image_->GetBoundsInScreen();
  cancel_animation_.reset(CreateCancelAnimation(
      animation_duration, kCancelAnimationFrameRate, this));
  cancel_animation_->Start();
}

void WebosDragDropClientWayland::Cleanup() {
  for (aura::client::DragDropClientObserver& observer : observers_)
    observer.OnDragEnded();
  if (drag_window_)
    drag_window_->RemoveObserver(this);
  drag_window_ = NULL;
  drag_data_ = NULL;
  // Cleanup can be called again while deleting DragDropTracker, so delete
  // the pointer with a local variable to avoid double free.
  std::unique_ptr<DragDropTracker> holder = std::move(drag_drop_tracker_);
}

void WebosDragDropClientWayland::SetCursor(gfx::NativeCursor cursor) {
  aura::client::CursorClient* cursor_client =
      aura::client::GetCursorClient(root_window_);
  if (cursor_client)
    cursor_client->SetCursor(cursor);
}

}  // namespace views
