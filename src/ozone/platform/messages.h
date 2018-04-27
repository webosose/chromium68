// Copyright 2013 Intel Corporation. All rights reserved.
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

// Multiply-included message file, hence no include guard here.

#include <string>
#include <vector>

#include "base/memory/shared_memory.h"
#include "base/strings/string16.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_message_utils.h"
#include "ipc/ipc_param_traits.h"
#include "ipc/param_traits_macros.h"
#include "ipc/neva/redefined_param_traits_macros.h"
#include "ozone/platform/input_content_type.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/ipc/geometry/gfx_param_traits.h"
#include "ui/gfx/ipc/gfx_param_traits.h"
#include "ui/gfx/ipc/skia/gfx_skia_param_traits.h"
#include "ui/platform_window/neva/window_group_configuration.h"
#include "ui/platform_window/neva/window_group_configuration_param_traits.h"
#include "ui/platform_window/neva/xinput_types.h"
#include "ui/views/widget/desktop_aura/neva/ui_constants.h"

#define IPC_MESSAGE_START LastIPCMsgStart

IPC_ENUM_TRAITS_MAX_VALUE(ui::EventFlags,
                          ui::EF_FORWARD_MOUSE_BUTTON)
IPC_ENUM_TRAITS_MAX_VALUE(ui::EventType,
                          ui::ET_LAST)
IPC_ENUM_TRAITS_MAX_VALUE(ui::InputContentType,
                          ui::INPUT_CONTENT_TYPE_DATE_TIME_FIELD)
IPC_ENUM_TRAITS_MAX_VALUE(ui::WidgetState,
                          ui::WidgetState::DESTROYED)
IPC_ENUM_TRAITS_MAX_VALUE(ui::WidgetType,
                          ui::WidgetType::TOOLTIP)
IPC_ENUM_TRAITS_MAX_VALUE(ui::KeyMask,
                          ui::KeyMask::kDefault)
IPC_ENUM_TRAITS_MAX_VALUE(ui::XInputKeySymbolType, ui::XINPUT_NATIVE_KEY_SYMBOL)
IPC_ENUM_TRAITS_MAX_VALUE(ui::XInputEventType, ui::XINPUT_RELEASE)

//------------------------------------------------------------------------------
// Browser Messages
// These messages are from the GPU to the browser process.

IPC_MESSAGE_CONTROL2(WaylandInput_InitializeXKB,  // NOLINT(readability/fn_size)
                     base::SharedMemoryHandle /*fd*/,
                     uint32_t /*size*/)

IPC_MESSAGE_CONTROL3(WaylandInput_KeyNotify,  // NOLINT(readability/fn_size)
                     ui::EventType /*type*/,
                     unsigned /*code*/,
                     int /*device_id*/)

IPC_MESSAGE_CONTROL3(  // NOLINT(readability/fn_size)
    WaylandInput_VirtualKeyNotify,
    ui::EventType /*type*/,
    uint32_t /*key*/,
    int /*device_id*/)

IPC_MESSAGE_CONTROL2(WaylandInput_MotionNotify,  // NOLINT(readability/fn_size)
                     float /*x*/,
                     float /*y*/)

IPC_MESSAGE_CONTROL5(WaylandInput_ButtonNotify,  // NOLINT(readability/fn_size)
                     unsigned /*handle*/,
                     ui::EventType /*type*/,
                     ui::EventFlags /*flags*/,
                     float /*x*/,
                     float /*y*/)

IPC_MESSAGE_CONTROL5(WaylandInput_TouchNotify,  // NOLINT(readability/fn_size)
                     ui::EventType /*type*/,
                     float /*x*/,
                     float /*y*/,
                     int32_t /*touch_id*/,
                     uint32_t /*time_stamp*/)

IPC_MESSAGE_CONTROL4(WaylandInput_AxisNotify,  // NOLINT(readability/fn_size)
                     float /*x*/,
                     float /*y*/,
                     int /*x_offset*/,
                     int /*y_offset*/)

IPC_MESSAGE_CONTROL3(WaylandInput_PointerEnter,  // NOLINT(readability/fn_size)
                     unsigned /*handle*/,
                     float /*x*/,
                     float /*y*/)

IPC_MESSAGE_CONTROL3(WaylandInput_PointerLeave,  // NOLINT(readability/fn_size)
                     unsigned /*handle*/,
                     float /*x*/,
                     float /*y*/)

IPC_MESSAGE_CONTROL3(WaylandOutput_ScreenChanged,  // NOLINT(readability/fn_size)
                     unsigned /*width*/,
                     unsigned /*height*/,
                     int /*rotation*/)

IPC_MESSAGE_CONTROL1(WaylandInput_CloseWidget,  // NOLINT(readability/fn_size)
                     unsigned /*handle*/)

IPC_MESSAGE_CONTROL3(WaylandWindow_Resized,  // NOLINT(readability/fn_size)
                     unsigned /* window handle */,
                     unsigned /* width */,
                     unsigned /* height */)

IPC_MESSAGE_CONTROL1(WaylandWindow_Unminimized,  // NOLINT(readability/fn_size)
                     unsigned /*handle*/)

IPC_MESSAGE_CONTROL1(WaylandWindow_DeActivated,  // NOLINT(readability/fn_size)
                     unsigned /*handle*/)

IPC_MESSAGE_CONTROL1(WaylandWindow_Activated,  // NOLINT(readability/fn_size)
                     unsigned /*handle*/)

IPC_MESSAGE_CONTROL2(WaylandInput_Commit,  // NOLINT(readability/fn_size)
                     unsigned,
                     std::string)

IPC_MESSAGE_CONTROL3(WaylandInput_PreeditChanged, // NOLINT(readability/
                     unsigned,                    //        fn_size)
                     std::string, std::string)

IPC_MESSAGE_CONTROL0(WaylandInput_PreeditEnd)  // NOLINT(readability/fn_size)

IPC_MESSAGE_CONTROL0(WaylandInput_PreeditStart)  // NOLINT(readability/fn_size)

IPC_MESSAGE_CONTROL3(WaylandInput_DeleteRange,  // NOLINT(readability/fn_size)
                     unsigned,
                     int32_t, /*index*/
                     uint32_t /*length*/)

IPC_MESSAGE_CONTROL5(WaylandInput_DragEnter,  // NOLINT(readability/fn_size)
                     unsigned /* window handle */,
                     float /* x */,
                     float /* y */,
                     std::vector<std::string> /* mime_types */,
                     uint32_t /* serial */)

IPC_MESSAGE_CONTROL2(WaylandInput_DragData,  // NOLINT(readability/fn_size)
                     unsigned /* window handle */,
                     base::FileDescriptor /* pipefd */)

IPC_MESSAGE_CONTROL1(WaylandInput_DragLeave,  // NOLINT(readability/fn_size)
                     unsigned /* window handle */)

IPC_MESSAGE_CONTROL4(WaylandInput_DragMotion,  // NOLINT(readability/fn_size)
                     unsigned /* window handle */,
                     float /* x */,
                     float /* y */,
                     uint32_t /* time */)

IPC_MESSAGE_CONTROL1(WaylandInput_DragDrop,  // NOLINT(readability/fn_size)
                     unsigned /* window handle */)

IPC_MESSAGE_CONTROL1(WaylandWindow_Close, // NOLINT(readability/fn_size)
                     unsigned /*handle*/)

IPC_MESSAGE_CONTROL1(WaylandWindow_Exposed, //NOLINT(readability/fn_size)
                     unsigned /*handle*/)

IPC_MESSAGE_CONTROL2(WaylandInput_InputPanelVisibilityChanged, //NOLINT(readability/fn_size)
                     unsigned /*handle*/,
                     bool /*visibility*/)

IPC_MESSAGE_CONTROL5(
    WaylandInput_InputPanelRectChanged,  // NOLINT(readability/fn_size)
    unsigned /*handle*/,
    int32_t /* x */,
    int32_t /* y */,
    uint32_t /* width */,
    uint32_t /* height */)

IPC_MESSAGE_CONTROL1(WaylandInput_KeyboardEnter, // NOLINT(readability/fn_size)
                     unsigned /*handle*/)

IPC_MESSAGE_CONTROL1(WaylandInput_KeyboardLeave, // NOLINT(readability/fn_size)
                     unsigned /*handle*/)

IPC_MESSAGE_CONTROL2(WaylandWindow_StateChanged, //NOLINT(readability/fn_size)
                     unsigned /* handle */,
                     ui::WidgetState /* new_state */)

IPC_MESSAGE_CONTROL2(WaylandWindow_StateAboutToChange, //NOLINT(readability/fn_size)
                     unsigned /* handle */,
                     ui::WidgetState /* state */)

//------------------------------------------------------------------------------
// GPU Messages
// These messages are from the Browser to the GPU process.

IPC_MESSAGE_CONTROL2(WaylandDisplay_State,  // NOLINT(readability/fn_size)
                     unsigned /* window handle */,
                     ui::WidgetState /*state*/)

IPC_MESSAGE_CONTROL2(WaylandDisplay_Create,  // NOLINT(readability/fn_size)
                     unsigned /* window handle */,
                     int /* surface id */)

IPC_MESSAGE_CONTROL5(WaylandDisplay_InitWindow,  // NOLINT(readability/fn_size)
                     unsigned /* window handle */,
                     unsigned /* window parent */,
                     int /* x */,
                     int /* y */,
                     ui::WidgetType /* window type */)

IPC_MESSAGE_CONTROL1(WaylandDisplay_DestroyWindow,  // NOLINT(readability/fn_size)
                     unsigned /* window handle */)

IPC_MESSAGE_CONTROL4(WaylandDisplay_MoveWindow,  // NOLINT(readability/fn_size)
                     unsigned /* window handle */,
                     unsigned /* window parent */,
                     ui::WidgetType /* window type */,
                     gfx::Rect /* rect */)

IPC_MESSAGE_CONTROL2(WaylandDisplay_Title,  // NOLINT(readability/fn_size)
                     unsigned /* window handle */,
                     base::string16 /* window title */)

IPC_MESSAGE_CONTROL5(WaylandDisplay_AddRegion,  // NOLINT(readability/fn_size)
                     unsigned /* window handle */,
                     int /* left */,
                     int /* top */,
                     int /* right */,
                     int /* bottom */)

IPC_MESSAGE_CONTROL5(WaylandDisplay_SubRegion,  // NOLINT(readability/fn_size)
                     unsigned /* window handle */,
                     int /* left */,
                     int /* top */,
                     int /* right */,
                     int /* bottom */)

IPC_MESSAGE_CONTROL2(WaylandDisplay_CursorSet,  // NOLINT(readability/fn_size)
                     std::vector<SkBitmap>,
                     gfx::Point)

IPC_MESSAGE_CONTROL1(WaylandDisplay_MoveCursor,  // NOLINT(readability/fn_size)
                     gfx::Point)

IPC_MESSAGE_CONTROL0(WaylandDisplay_ImeReset)  // NOLINT(readability/fn_size)

IPC_MESSAGE_CONTROL1(WaylandDisplay_ShowInputPanel,  // NOLINT(readability/fn_size)
                     unsigned /* window handle */)

IPC_MESSAGE_CONTROL0(WaylandDisplay_HideInputPanel)  // NOLINT(readability/
                                                     //         fn_size)

IPC_MESSAGE_CONTROL3(WaylandDisplay_SetInputContentType,  // NOLINT(readability/fn_size)
                     ui::InputContentType /* text_input_type */,
                     int                  /* text_input_flags */,
                     unsigned /* window handle */)

IPC_MESSAGE_CONTROL1(WaylandDisplay_RequestDragData,  // NOLINT(readability/
                     std::string /* mime_type */)     //        fn_size)

IPC_MESSAGE_CONTROL1(  // NOLINT(readability/fn_size)
    WaylandDisplay_RequestSelectionData,
    std::string /* mime_type */)

IPC_MESSAGE_CONTROL2(WaylandDisplay_DragWillBeAccepted,  // NOLINT(readability/
                     uint32_t /* serial */,              //        fn_size)
                     std::string /* mime_type */)

IPC_MESSAGE_CONTROL1(WaylandDisplay_DragWillBeRejected,  // NOLINT(readability/
                     uint32_t /* serial */)              //        fn_size)

IPC_MESSAGE_CONTROL3(
    WaylandDisplay_SetWindowProperty,  // NOLINT(readability/fn_size)
    unsigned /*handle*/,
    std::string /*name*/,
    std::string /*value*/)

IPC_MESSAGE_CONTROL3(WaylandDisplay_SetSurroundingText,
                     std::string /*text*/,
                     unsigned /*cursor_position*/,
                     unsigned /*anchor_position*/)

IPC_MESSAGE_CONTROL1(WaylandDisplay_XInputActivate, std::string /* text*/)
IPC_MESSAGE_CONTROL0(WaylandDisplay_XInputDeactivate)
IPC_MESSAGE_CONTROL3(WaylandDisplay_XInputInvokeAction,
                     unsigned /*key_sym*/,
                     ui::XInputKeySymbolType /*symbol_type*/,
                     ui::XInputEventType /*event_type*/)

IPC_MESSAGE_CONTROL2(WaylandDisplay_SetInputRegion, //NOLINT(readability/fn_size)
                     unsigned /* handle */,
                     std::vector<gfx::Rect> /* region */)

IPC_MESSAGE_CONTROL2(WaylandDisplay_SetGroupKeyMask, //NOLINT(readability/fn_size)
                     unsigned /* handle */,
                     ui::KeyMask /* key_mask */)

IPC_MESSAGE_CONTROL3(WaylandDisplay_SetKeyMask, //NOLINT(readability/fn_size)
                     unsigned /* handle */,
                     ui::KeyMask /* key_mask */,
                     bool /* set */)

IPC_MESSAGE_CONTROL2(
    WaylandDisplay_CreateWindowGroup,  // NOLINT(readability/fn_size)
    unsigned /*handle*/,
    ui::WindowGroupConfiguration /*config*/)

IPC_MESSAGE_CONTROL3(
    WaylandDisplay_AttachToWindowGroup,  // NOLINT(readability/fn_size)
    unsigned /*handle*/,
    std::string /*name*/,
    std::string /*layer*/)

IPC_MESSAGE_CONTROL1(
    WaylandDisplay_FocusWindowGroupOwner,  // NOLINT(readability/fn_size)
    unsigned /*handle*/)

IPC_MESSAGE_CONTROL1(
    WaylandDisplay_FocusWindowGroupLayer,  // NOLINT(readability/fn_size)
    unsigned /*handle*/)

IPC_MESSAGE_CONTROL1(
    WaylandDisplay_DetachWindowGroup,  // NOLINT(readability/fn_size)
    unsigned /*handle*/)
