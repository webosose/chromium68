// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMULATOR_EMULATOR_EXPORT_H_
#define EMULATOR_EMULATOR_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(EMULATOR_IMPLEMENTATION)
#define EMULATOR_EXPORT __declspec(dllexport)
#else
#define EMULATOR_EXPORT __declspec(dllimport)
#endif  // defined(EMULATOR_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(EMULATOR_IMPLEMENTATION)
#define EMULATOR_EXPORT __attribute__((visibility("default")))
#else
#define EMULATOR_EXPORT
#endif  // defined(EMULATOR_IMPLEMENTATION)
#endif

#else  // defined(COMPONENT_BUILD)
#define EMULATOR_EXPORT
#endif

#endif  // EMULATOR_EMULATOR_EXPORT_H_
