// Copyright (c) 2019 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Logic is partially copied from DCLayerOverlayProcessor in dc_layer_overlay.h

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_NEVA_NEVA_LAYER_OVERLAY_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_NEVA_NEVA_LAYER_OVERLAY_H_

#include "base/containers/flat_map.h"
#include "components/viz/common/quads/render_pass.h"

namespace viz {
class DisplayResourceProvider;

// Holds all information necessary to construct a NevaLayer from a DrawQuad.
class NevaLayerOverlay {
 public:
  NevaLayerOverlay();
  NevaLayerOverlay(const NevaLayerOverlay& other);
  ~NevaLayerOverlay();

  // The bounds for the NevaLayer in pixels.
  gfx::RectF bounds_rect;
};

class NevaLayerOverlayProcessor {
 public:
  NevaLayerOverlayProcessor();
  ~NevaLayerOverlayProcessor();

  void Process(DisplayResourceProvider* resource_provider,
               const gfx::RectF& display_rect,
               RenderPassList* render_passes,
               gfx::Rect* overlay_damage_rect,
               gfx::Rect* damage_rect);

private:

  bool FromDrawQuad(DisplayResourceProvider* resource_provider,
                             const gfx::RectF& display_rect,
                             QuadList::ConstIterator quad_list_begin,
                             QuadList::ConstIterator quad,
                             NevaLayerOverlay* neva_layer_overlay);

  // Returns an iterator to the element after |it|.
  QuadList::Iterator ProcessRenderPassDrawQuad(RenderPass* render_pass,
                                               gfx::Rect* damage_rect,
                                               QuadList::Iterator it);

  void ProcessRenderPass(DisplayResourceProvider* resource_provider,
                         const gfx::RectF& display_rect,
                         RenderPass* render_pass,
                         bool is_root,
                         gfx::Rect* overlay_damage_rect,
                         gfx::Rect* damage_rect);

  bool ProcessForUnderlay(const gfx::RectF& display_rect,
                          RenderPass* render_pass,
                          const QuadList::Iterator& it,
                          bool is_root,
                          gfx::Rect* damage_rect,
                          gfx::Rect* this_frame_underlay_rect,
                          NevaLayerOverlay* neva_layer);

  bool processed_overlay_in_frame_ = false;

  // Store information about punch-through rectangles for non-root
  // RenderPasses. These rectangles are used to clear the corresponding areas
  // in parent renderpasses.
  struct PunchThroughRect {
    gfx::Rect rect;
    gfx::Transform transform_to_target;
    float opacity;
  };

  base::flat_map<RenderPassId, std::vector<PunchThroughRect>> pass_info_;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_NEVA_NEVA_LAYER_OVERLAY_H_
