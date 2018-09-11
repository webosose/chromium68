// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_OZONE_WEB_UI_H_
#define CHROME_BROWSER_UI_OZONE_WEB_UI_H_

#include <map>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/observer_list.h"
#include "build/buildflag.h"
#include "ozone/platform/ozone_export_wayland.h"
#include "ui/base/ime/linux/linux_input_method_context.h"
#include "ui/base/ime/linux/text_edit_key_bindings_delegate_auralinux.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/font_render_params.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/image/image.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/linux_ui/linux_ui.h"
#include "ui/views/window/frame_buttons.h"

class SkBitmap;

namespace gfx {
class Image;
}

namespace ui {
class OzoneGpuPlatformSupportHost;
class InputMethodContextManager;
}

using ui::TextEditCommandAuraLinux;

namespace views {
class Border;
class LabelButton;
class View;
class NativeThemeChangeObserver;
class WindowButtonOrderObserver;
// Interface to Wayland desktop features.
//
class OZONE_WAYLAND_EXPORT OzoneWebUI : public views::LinuxUI {
 public:
  OzoneWebUI();
  ~OzoneWebUI() override;

  // ui::LinuxInputMethodContextFactory:
  std::unique_ptr<ui::LinuxInputMethodContext> CreateInputMethodContext(
      ui::LinuxInputMethodContextDelegate* delegate, bool is_simple)
      const override;

  std::unique_ptr<ui::LinuxInputMethodContext> CreateInputMethodContext(
      ui::LinuxInputMethodContextDelegate* delegate, unsigned handle, bool is_simple)
      const override;

  // gfx::LinuxFontDelegate:
  gfx::FontRenderParams GetDefaultFontRenderParams() const override;

  // ui::LinuxShellDialog:
  ui::SelectFileDialog* CreateSelectFileDialog(
      ui::SelectFileDialog::Listener* listener,
      std::unique_ptr<ui::SelectFilePolicy> policy) const override;

  void GetDefaultFontDescription(
      std::string* family_out,
      int* size_pixels_out,
      int* style_out,
      gfx::Font::Weight* weight_out,
      gfx::FontRenderParams* params_out) const override;

  // ui::LinuxUI:
  void Initialize() override;

  // These methods are not needed
  bool GetColor(int id, SkColor* color) const override;
  SkColor GetFocusRingColor() const override;
  SkColor GetThumbActiveColor() const override;
  SkColor GetThumbInactiveColor() const override;
  SkColor GetTrackColor() const override;
  SkColor GetActiveSelectionBgColor() const override;
  SkColor GetActiveSelectionFgColor() const override;
  SkColor GetInactiveSelectionBgColor() const override;
  SkColor GetInactiveSelectionFgColor() const override;
  base::TimeDelta GetCursorBlinkInterval() const override;
  ui::NativeTheme* GetNativeTheme(aura::Window* window) const override;
  void SetNativeThemeOverride(const NativeThemeGetter& callback) override;
  bool GetDefaultUsesSystemTheme() const override;
  void SetDownloadCount(int count) const override;
  void SetProgressFraction(float percentage) const override;
  bool IsStatusIconSupported() const override;
  std::unique_ptr<StatusIconLinux> CreateLinuxStatusIcon(
      const gfx::ImageSkia& image,
      const base::string16& tool_tip) const override;
  gfx::Image GetIconForContentType(const std::string& content_type,
                                   int size) const override;
  std::unique_ptr<Border> CreateNativeBorder(
      views::LabelButton* owning_button,
      std::unique_ptr<views::LabelButtonBorder> border) override;
  void AddWindowButtonOrderObserver(
      WindowButtonOrderObserver* observer) override;
  void RemoveWindowButtonOrderObserver(
      WindowButtonOrderObserver* observer) override;
  NonClientWindowFrameAction GetNonClientWindowFrameAction(
      NonClientWindowFrameActionSourceType source) override;

  void NotifyWindowManagerStartupComplete() override;

  bool MatchEvent(const ui::Event& event,
                  std::vector<TextEditCommandAuraLinux>* commands) override;

  void UpdateDeviceScaleFactor() override;
  float GetDeviceScaleFactor() const override;
  bool GetTint(int id, color_utils::HSL* tint) const override;
  void AddDeviceScaleFactorObserver(DeviceScaleFactorObserver* observer) override;
  void RemoveDeviceScaleFactorObserver(DeviceScaleFactorObserver* observer) override;
  bool PreferDarkTheme() const override;
#if BUILDFLAG(ENABLE_NATIVE_WINDOW_NAV_BUTTONS)
  std::unique_ptr<NavButtonProvider> CreateNavButtonProvider() override;
#endif

  base::flat_map<std::string, std::string> GetKeyboardLayoutMap() override;

 private:
  gfx::FontRenderParams params_;
  std::unique_ptr<ui::InputMethodContextManager> input_method_context_manager_;
  ui::OzoneGpuPlatformSupportHost* host_;
  DISALLOW_COPY_AND_ASSIGN(OzoneWebUI);
};

}  // namespace views

views::LinuxUI* BuildWebUI();

#endif  // CHROME_BROWSER_UI_OZONE_WEB_UI_H_
