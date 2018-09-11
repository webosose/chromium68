/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright 2014 The Chromium Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_INJECTED_STYLE_SHEETS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_INJECTED_STYLE_SHEETS_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

enum StyleInjectionTarget { InjectStyleInAllFrames, InjectStyleInTopFrameOnly };

class InjectedStyleSheetEntry {
  USING_FAST_MALLOC(InjectedStyleSheetEntry);
public:
  InjectedStyleSheetEntry(const String& source, const Vector<String>& whitelist,
                          StyleInjectionTarget injectedFrames)
    : source_(source)
    , white_list_(whitelist)
    , injected_frames_(injectedFrames) {
    }

  const String& Source() const { return source_; }
  const Vector<String>& WhiteList() const { return white_list_; }
  StyleInjectionTarget InjectedFrames() const { return injected_frames_; }

private:
  String source_;
  Vector<String> white_list_;
  StyleInjectionTarget injected_frames_;
};

using InjectedStyleSheetEntryVector = Vector<std::unique_ptr<InjectedStyleSheetEntry>>;

class CORE_EXPORT InjectedStyleSheets {
public:
  static InjectedStyleSheets& Instance();

  void Add(const String& source, const Vector<String>& whitelist, StyleInjectionTarget);
  void RemoveAll();

  const InjectedStyleSheetEntryVector& Entries() const { return entries_; }

private:
  InjectedStyleSheets() { }
  void InvalidateInjectedStyleSheetCacheInAllFrames();

  InjectedStyleSheetEntryVector entries_;
};

} // namespace blink

#endif // THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_INJECTED_STYLE_SHEETS_H_
