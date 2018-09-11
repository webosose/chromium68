// Copyright (c) 2018 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.

#include "ui/views/widget/desktop_aura/desktop_screen_headless.h"

#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"

namespace views {

namespace {

const int64_t kHeadlessDisplayId = 0;
const float kHeadlessScaleFactor = 1.0f;
const int kHeadlessDisplayWidth  = 1920;
const int kHeadlessDisplayHeight = 1080;

}  // namespace

DesktopScreenHeadless::DesktopScreenHeadless() {
  display::Display display(kHeadlessDisplayId);
  display.SetScaleAndBounds(kHeadlessScaleFactor,
                            gfx::Rect(gfx::Size(kHeadlessDisplayWidth,
                                                kHeadlessDisplayHeight)));
  displays_.push_back(display);
}

DesktopScreenHeadless::~DesktopScreenHeadless() {
}

gfx::Point DesktopScreenHeadless::GetCursorScreenPoint() {
  NOTIMPLEMENTED();
  return gfx::Point();
}

bool DesktopScreenHeadless::IsWindowUnderCursor(gfx::NativeWindow window) {
  NOTIMPLEMENTED();
  return false;
}

gfx::NativeWindow DesktopScreenHeadless::GetWindowAtScreenPoint(
    const gfx::Point& point) {
  NOTIMPLEMENTED();
  return nullptr;
}

int DesktopScreenHeadless::GetNumDisplays() const {
  return displays_.size();
}

const std::vector<display::Display>&
DesktopScreenHeadless::GetAllDisplays() const {
  return displays_;
}

display::Display DesktopScreenHeadless::GetDisplayNearestWindow(
    gfx::NativeView window) const {
  NOTIMPLEMENTED();
  return GetPrimaryDisplay();
}

display::Display DesktopScreenHeadless::GetDisplayNearestPoint(
    const gfx::Point& point) const {
  NOTIMPLEMENTED();
  return GetPrimaryDisplay();
}

display::Display DesktopScreenHeadless::GetDisplayMatching(
    const gfx::Rect& match_rect) const {
  NOTIMPLEMENTED();
  return GetPrimaryDisplay();
}

display::Display DesktopScreenHeadless::GetPrimaryDisplay() const {
  return displays_.front();
}

void DesktopScreenHeadless::AddObserver(display::DisplayObserver* observer) {
  NOTIMPLEMENTED();
}

void DesktopScreenHeadless::RemoveObserver(display::DisplayObserver* observer) {
  NOTIMPLEMENTED();
}

}  // namespace views
