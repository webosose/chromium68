/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright 2014 The Chromium Authors. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "third_party/blink/renderer/core/page/injected_style_sheets.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/css/style_engine.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"

namespace blink {

// static
InjectedStyleSheets& InjectedStyleSheets::Instance() {
  DEFINE_STATIC_LOCAL(InjectedStyleSheets, instance, ());
  return instance;
}

void InjectedStyleSheets::Add(const String& source, const Vector<String>& whitelist,
                              StyleInjectionTarget injectIn) {
  entries_.push_back(std::move(new InjectedStyleSheetEntry(source, whitelist, injectIn)));
  InvalidateInjectedStyleSheetCacheInAllFrames();
}

void InjectedStyleSheets::RemoveAll() {
  entries_.clear();
  InvalidateInjectedStyleSheetCacheInAllFrames();
}

void InjectedStyleSheets::InvalidateInjectedStyleSheetCacheInAllFrames() {
  // Clear our cached sheets and have them just reparse.
  const Page::PageSet& pages = Page::OrdinaryPages();
  for (const Page* page : pages) {
    for (Frame* frame = page->MainFrame(); frame; frame = frame->Tree().TraverseNext()) {
      if (frame->IsLocalFrame())
        ToLocalFrame(frame)->GetDocument()->GetStyleEngine().InvalidateInjectedStyleSheetCache();
    }
  }
}

} // namespace blink
