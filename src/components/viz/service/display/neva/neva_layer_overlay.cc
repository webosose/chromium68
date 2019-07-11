// Copyright (c) 2019 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Logic is partially copied from DCLayerOverlayProcessor in dc_layer_overlay.cc

#include "components/viz/service/display/neva/neva_layer_overlay.h"

#include "cc/base/math_util.h"
#include "components/viz/common/quads/render_pass_draw_quad.h"
#include "components/viz/common/quads/solid_color_draw_quad.h"
#include "components/viz/service/display/display_resource_provider.h"
#include "ui/gfx/geometry/rect_conversions.h"

namespace viz {

namespace {

bool FromSolidColorDrawQuad(
    DisplayResourceProvider* resource_provider,
    const SolidColorDrawQuad* quad,
    NevaLayerOverlay* neva_layer_overlay) {

  if (quad->is_overlay_for_video_hole)
    return true;

  return false;
}

// This returns the smallest rectangle in target space that contains the quad.
gfx::RectF ClippedQuadRectangle(const DrawQuad* quad) {
  gfx::RectF quad_rect = cc::MathUtil::MapClippedRect(
      quad->shared_quad_state->quad_to_target_transform,
      gfx::RectF(quad->rect));
  if (quad->shared_quad_state->is_clipped)
    quad_rect.Intersect(gfx::RectF(quad->shared_quad_state->clip_rect));
  return quad_rect;
}

} // namespace

NevaLayerOverlay::NevaLayerOverlay() {}

NevaLayerOverlay::NevaLayerOverlay(const NevaLayerOverlay& other) = default;

NevaLayerOverlay::~NevaLayerOverlay() {}

NevaLayerOverlayProcessor::NevaLayerOverlayProcessor() = default;

NevaLayerOverlayProcessor::~NevaLayerOverlayProcessor() = default;

bool NevaLayerOverlayProcessor::FromDrawQuad(
    DisplayResourceProvider* resource_provider,
    const gfx::RectF& display_rect,
    QuadList::ConstIterator quad_list_begin,
    QuadList::ConstIterator quad,
    NevaLayerOverlay* neva_layer_overlay) {
  if (quad->shared_quad_state->blend_mode != SkBlendMode::kSrcOver)
    return false;

  bool success{false};
  switch (quad->material) {
    case DrawQuad::SOLID_COLOR:
      success =
          FromSolidColorDrawQuad(resource_provider,
                      SolidColorDrawQuad::MaterialCast(*quad),
                      neva_layer_overlay);
      break;
    default:
      return false;
  }

  if (!success)
    return false;

  neva_layer_overlay->bounds_rect = gfx::RectF(quad->rect);

  return success;
}

void NevaLayerOverlayProcessor::Process(
    DisplayResourceProvider* resource_provider,
    const gfx::RectF& display_rect,
    RenderPassList* render_passes,
    gfx::Rect* overlay_damage_rect,
    gfx::Rect* damage_rect) {
  DCHECK(pass_info_.empty());
  processed_overlay_in_frame_ = false;

  for (auto& pass : *render_passes) {
    bool is_root = (pass == render_passes->back());
    ProcessRenderPass(resource_provider, display_rect, pass.get(), is_root,
                      overlay_damage_rect,
                      is_root ? damage_rect : &pass->damage_rect);
  }
  pass_info_.clear();
}

QuadList::Iterator NevaLayerOverlayProcessor::ProcessRenderPassDrawQuad(
    RenderPass* render_pass,
    gfx::Rect* damage_rect,
    QuadList::Iterator it) {
  DCHECK_EQ(DrawQuad::RENDER_PASS, it->material);
  const RenderPassDrawQuad* rpdq = RenderPassDrawQuad::MaterialCast(*it);

  ++it;
  // Check if this quad is broken to avoid corrupting pass_info.
  if (rpdq->render_pass_id == render_pass->id)
    return it;
  if (!pass_info_.count(rpdq->render_pass_id))
    return it;
  pass_info_[render_pass->id] = std::vector<PunchThroughRect>();
  auto& pass_info = pass_info_[rpdq->render_pass_id];

  const SharedQuadState* original_shared_quad_state = rpdq->shared_quad_state;

  it = render_pass->quad_list
           .InsertBeforeAndInvalidateAllPointers<SolidColorDrawQuad>(
               it, pass_info.size());
  rpdq = nullptr;
  for (size_t i = 0; i < pass_info.size(); i++, ++it) {
    auto& punch_through = pass_info[i];
    SharedQuadState* new_shared_quad_state =
        render_pass->shared_quad_state_list
            .AllocateAndConstruct<SharedQuadState>();
    gfx::Transform new_transform(
        original_shared_quad_state->quad_to_target_transform,
        punch_through.transform_to_target);
    float new_opacity =
        punch_through.opacity * original_shared_quad_state->opacity;
    new_shared_quad_state->SetAll(new_transform, punch_through.rect,
                                  punch_through.rect, punch_through.rect, false,
                                  true, new_opacity, SkBlendMode::kDstOut, 0);
    auto* solid_quad = static_cast<SolidColorDrawQuad*>(*it);
    solid_quad->SetAll(new_shared_quad_state, punch_through.rect,
                       punch_through.rect, false, 0xff000000, true);
    damage_rect->Union(gfx::ToEnclosingRect(ClippedQuadRectangle(solid_quad)));

    // Add transformed info to list in case this renderpass is included in
    // another pass.
    PunchThroughRect info;
    info.rect = punch_through.rect;
    info.transform_to_target = new_transform;
    info.opacity = new_opacity;
    pass_info_[render_pass->id].push_back(info);
  }
  return it;
}

void NevaLayerOverlayProcessor::ProcessRenderPass(
    DisplayResourceProvider* resource_provider,
    const gfx::RectF& display_rect,
    RenderPass* render_pass,
    bool is_root,
    gfx::Rect* overlay_damage_rect,
    gfx::Rect* damage_rect) {
  gfx::Rect this_frame_underlay_rect;
  QuadList* quad_list = &render_pass->quad_list;

  auto next_it = quad_list->begin();
  for (auto it = quad_list->begin(); it != quad_list->end(); it = next_it) {
    next_it = it;
    ++next_it;
    // next_it may be modified inside the loop if methods modify the quad list
    // and invalidate iterators to it.

    if (it->material == DrawQuad::RENDER_PASS) {
      next_it = ProcessRenderPassDrawQuad(render_pass, damage_rect, it);
    }

    NevaLayerOverlay neva_layer;
    if (!FromDrawQuad(resource_provider, display_rect,
                                        quad_list->begin(), it, &neva_layer)) {
      continue;
    }

    ProcessForUnderlay(display_rect, render_pass, it, is_root,
                                  damage_rect, &this_frame_underlay_rect,
                                  &neva_layer);
  }
}

bool NevaLayerOverlayProcessor::ProcessForUnderlay(
    const gfx::RectF& display_rect,
    RenderPass* render_pass,
    const QuadList::Iterator& it,
    bool is_root,
    gfx::Rect* damage_rect,
    gfx::Rect* this_frame_underlay_rect,
    NevaLayerOverlay* neva_layer) {
  // Do not process root passes
  if (is_root)
    return false;

  const SharedQuadState* shared_quad_state = it->shared_quad_state;

  // Store vide rect info so that other passes can create
  // punch through quads
  PunchThroughRect info;
  info.rect = gfx::ToEnclosingRect(neva_layer->bounds_rect);
  info.transform_to_target = shared_quad_state->quad_to_target_transform;
  info.opacity = shared_quad_state->opacity;
  pass_info_[render_pass->id].push_back(info);

  return true;
}

}  // namespace viz
