// Copyright (c) 2018 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.

#ifndef UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_SCREEN_HEADLESS_H__
#define UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_SCREEN_HEADLESS_H__

#include <vector>

#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/native_widget_types.h"

namespace gfx {
class Point;
class Rect;
}

namespace views {

class DesktopScreenHeadless : public display::Screen {
 public:
  DesktopScreenHeadless();
  ~DesktopScreenHeadless() override;

 private:
  // Overridden from display::Screen:
  gfx::Point GetCursorScreenPoint() override;
  bool IsWindowUnderCursor(gfx::NativeWindow window) override;
  gfx::NativeWindow GetWindowAtScreenPoint(const gfx::Point& point) override;
  int GetNumDisplays() const override;
  const std::vector<display::Display>& GetAllDisplays() const override;
  display::Display GetDisplayNearestWindow(
    gfx::NativeView window) const override;
  display::Display GetDisplayNearestPoint(
    const gfx::Point& point) const override;
  display::Display GetDisplayMatching(
    const gfx::Rect& match_rect) const override;
  display::Display GetPrimaryDisplay() const override;
  void AddObserver(display::DisplayObserver* observer) override;
  void RemoveObserver(display::DisplayObserver* observer) override;

  // The fake display object we present to chrome.
  std::vector<display::Display> displays_;
  DISALLOW_COPY_AND_ASSIGN(DesktopScreenHeadless);
};

}  // namespace views

#endif  // UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_SCREEN_HEADLESS_H__
