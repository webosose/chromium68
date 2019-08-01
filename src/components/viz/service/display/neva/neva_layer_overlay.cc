// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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

bool IsVideoHoleSolidColorDrawQuad(DisplayResourceProvider* resource_provider,
                                   const SolidColorDrawQuad* quad) {
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

// Find a rectangle containing all the quads in a list that occlude the area
// in target_quad.
gfx::RectF GetOcclusionBounds(const gfx::RectF& target_quad,
                              QuadList::ConstIterator quad_list_begin,
                              QuadList::ConstIterator quad_list_end) {
  gfx::RectF occlusion_bounding_box;
  for (auto overlap_iter = quad_list_begin; overlap_iter != quad_list_end;
       ++overlap_iter) {
    float opacity = overlap_iter->shared_quad_state->opacity;
    if (opacity < std::numeric_limits<float>::epsilon())
      continue;
    const DrawQuad* quad = *overlap_iter;
    gfx::RectF overlap_rect = ClippedQuadRectangle(quad);
    if (quad->material == DrawQuad::SOLID_COLOR) {
      SkColor color = SolidColorDrawQuad::MaterialCast(quad)->color;
      float alpha = (SkColorGetA(color) * (1.0f / 255.0f)) * opacity;
      if (quad->ShouldDrawWithBlending() &&
          alpha < std::numeric_limits<float>::epsilon())
        continue;
    }
    overlap_rect.Intersect(target_quad);
    if (!overlap_rect.IsEmpty()) {
      occlusion_bounding_box.Union(overlap_rect);
    }
  }
  return occlusion_bounding_box;
}

}  // namespace

NevaLayerOverlayProcessor::NevaLayerOverlayProcessor() = default;

NevaLayerOverlayProcessor::~NevaLayerOverlayProcessor() = default;

bool NevaLayerOverlayProcessor::IsVideoHoleDrawQuad(
    DisplayResourceProvider* resource_provider,
    const gfx::RectF& display_rect,
    QuadList::ConstIterator quad_list_begin,
    QuadList::ConstIterator quad) {
  if (quad->shared_quad_state->blend_mode != SkBlendMode::kSrcOver)
    return false;

  bool success{false};
  switch (quad->material) {
    case DrawQuad::SOLID_COLOR:
      success = IsVideoHoleSolidColorDrawQuad(
          resource_provider, SolidColorDrawQuad::MaterialCast(*quad));
      break;
    default:
      return false;
  }

  if (!success)
    return false;

  return success;
}

void NevaLayerOverlayProcessor::AddPunchThroughRectIfNeeded(
    RenderPassId id,
    const gfx::Rect& rect) {
  bool add_punch_through_rect = true;
  const auto& punch_through_rects = pass_punch_through_rects_[id];
  for (const gfx::Rect& punch_through_rect : punch_through_rects) {
    if (punch_through_rect == rect) {
      add_punch_through_rect = false;
      break;
    }
  }

  if (add_punch_through_rect)
    pass_punch_through_rects_[id].push_back(rect);
}

void NevaLayerOverlayProcessor::Process(
    DisplayResourceProvider* resource_provider,
    const gfx::RectF& display_rect,
    RenderPassList* render_passes,
    gfx::Rect* overlay_damage_rect,
    gfx::Rect* damage_rect) {
  processed_overlay_in_frame_ = false;
  pass_punch_through_rects_.clear();
  for (auto& pass : *render_passes) {
    bool is_root = (pass == render_passes->back());
    ProcessRenderPass(resource_provider, display_rect, pass.get(), is_root,
                      overlay_damage_rect,
                      is_root ? damage_rect : &pass->damage_rect);
  }
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
  // |pass_punch_through_rects_| will be empty unless non-root overlays are
  // enabled.
  if (!pass_punch_through_rects_.count(rpdq->render_pass_id))
    return it;

  // Punch holes through for all child video quads that will be displayed in
  // underlays. This doesn't work perfectly in all cases - it breaks with
  // complex overlap or filters - but it's needed to be able to display these
  // videos at all. The EME spec allows that some HTML rendering capabilities
  // may be unavailable for EME videos.
  //
  // The solid color quads are inserted after the RPDQ, so they'll be drawn
  // before it and will only cut out contents behind it. A kDstOut solid color
  // quad is used with an accumulated opacity to do the hole punching, because
  // with premultiplied alpha that reduces the opacity of the current content
  // by the opacity of the layer.
  const auto& punch_through_rects =
      pass_punch_through_rects_[rpdq->render_pass_id];
  const SharedQuadState* original_shared_quad_state = rpdq->shared_quad_state;

  bool should_blend =
      rpdq->ShouldDrawWithBlending() &&
      original_shared_quad_state->blend_mode == SkBlendMode::kSrcOver &&
      original_shared_quad_state->opacity < 1.f;

  // The iterator was advanced above so InsertBefore inserts after the RPDQ.
  it = render_pass->quad_list
           .InsertBeforeAndInvalidateAllPointers<SolidColorDrawQuad>(
               it, punch_through_rects.size());
  rpdq = nullptr;
  for (const gfx::Rect& punch_through_rect : punch_through_rects) {
    // Copy shared state from RPDQ to get the same clip rect.
    SharedQuadState* new_shared_quad_state =
        render_pass->shared_quad_state_list
            .AllocateAndCopyFrom<SharedQuadState>(original_shared_quad_state);

    SkBlendMode new_blend_mode = should_blend
                                     ? SkBlendMode::kDstOut
                                     : original_shared_quad_state->blend_mode;
    new_shared_quad_state->blend_mode = new_blend_mode;

    auto* solid_quad = static_cast<SolidColorDrawQuad*>(*it++);
    solid_quad->SetAll(
        new_shared_quad_state, punch_through_rect, punch_through_rect, false,
        should_blend ? SK_ColorBLACK : SK_ColorTRANSPARENT, true);

    gfx::Rect clipped_quad_rect =
        gfx::ToEnclosingRect(ClippedQuadRectangle(solid_quad));
    // Propagate punch through rect as damage up the stack of render passes.
    // TODO(sunnyps): We should avoid this extra damage if we knew that the
    // video (in child render surface) was the only thing damaging this
    // render surface.
    damage_rect->Union(clipped_quad_rect);

    // Add transformed info to list in case this renderpass is included in
    // another pass.
    pass_punch_through_rects_[render_pass->id].push_back(clipped_quad_rect);
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
  gfx::Rect this_frame_underlay_occlusion;

  QuadList* quad_list = &render_pass->quad_list;
  auto next_it = quad_list->begin();
  for (auto it = quad_list->begin(); it != quad_list->end(); it = next_it) {
    next_it = it;
    ++next_it;
    // next_it may be modified inside the loop if methods modify the quad list
    // and invalidate iterators to it.

    if (it->material == DrawQuad::RENDER_PASS) {
      next_it = ProcessRenderPassDrawQuad(render_pass, damage_rect, it);
      continue;
    }

    if (!IsVideoHoleDrawQuad(resource_provider, display_rect,
                             quad_list->begin(), it)) {
      continue;
    }

    // These rects are in quad target space.
    gfx::Rect quad_rectangle_in_target_space =
        gfx::ToEnclosingRect(ClippedQuadRectangle(*it));
    gfx::RectF occlusion_bounding_box = GetOcclusionBounds(
        gfx::RectF(quad_rectangle_in_target_space), quad_list->begin(), it);
    bool processed_overlay = false;

    if (ProcessForUnderlay(
            display_rect, render_pass, quad_rectangle_in_target_space,
            occlusion_bounding_box, it, is_root, damage_rect,
            &this_frame_underlay_rect, &this_frame_underlay_occlusion)) {
      processed_overlay = true;
    }

    if (processed_overlay) {
      gfx::Rect rect_in_root = cc::MathUtil::MapEnclosingClippedRect(
          render_pass->transform_to_root_target,
          quad_rectangle_in_target_space);
      overlay_damage_rect->Union(rect_in_root);

      // Only allow one overlay unless non-root overlays are enabled.
      processed_overlay_in_frame_ = true;
    }
  }
  if (is_root) {
    damage_rect->Intersect(gfx::ToEnclosingRect(display_rect));
    previous_display_rect_ = display_rect;
    previous_frame_underlay_rect_ = this_frame_underlay_rect;
    previous_frame_underlay_occlusion_ = this_frame_underlay_occlusion;
  }
}

bool NevaLayerOverlayProcessor::ProcessForUnderlay(
    const gfx::RectF& display_rect,
    RenderPass* render_pass,
    const gfx::Rect& quad_rectangle,
    const gfx::RectF& occlusion_bounding_box,
    const QuadList::Iterator& it,
    bool is_root,
    gfx::Rect* damage_rect,
    gfx::Rect* this_frame_underlay_rect,
    gfx::Rect* this_frame_underlay_occlusion) {
  if ((it->shared_quad_state->opacity < 1.0))
    return false;

  if (processed_overlay_in_frame_)
    return false;

  const SharedQuadState* shared_quad_state = it->shared_quad_state;
  gfx::Rect rect = it->visible_rect;

  bool display_rect_changed = (display_rect != previous_display_rect_);
  bool underlay_rect_changed =
      (quad_rectangle != previous_frame_underlay_rect_);
  bool is_axis_aligned =
      shared_quad_state->quad_to_target_transform.Preserves2dAxisAlignment();

  if (is_root && !processed_overlay_in_frame_ && is_axis_aligned &&
      !underlay_rect_changed && !display_rect_changed) {
    // If this underlay rect is the same as for last frame, subtract its area
    // from the damage of the main surface, as the cleared area was already
    // cleared last frame. Add back the damage from the occluded area for this
    // and last frame, as that may have changed.
    gfx::Rect occluding_damage_rect = *damage_rect;
    damage_rect->Subtract(quad_rectangle);

    gfx::Rect occlusion = gfx::ToEnclosingRect(occlusion_bounding_box);
    occlusion.Union(previous_frame_underlay_occlusion_);

    occluding_damage_rect.Intersect(quad_rectangle);
    occluding_damage_rect.Intersect(occlusion);

    damage_rect->Union(occluding_damage_rect);
  } else {
    // Entire replacement quad must be redrawn.
    // TODO(sunnyps): We should avoid this extra damage if we knew that the
    // video was the only thing damaging this render surface.
    damage_rect->Union(quad_rectangle);
  }

  // We only compare current frame's first root pass underlay with the previous
  // frame's first root pass underlay. Non-opaque regions can have different
  // alpha from one frame to another so this optimization doesn't work.
  if (is_root && !processed_overlay_in_frame_ && is_axis_aligned) {
    *this_frame_underlay_rect = quad_rectangle;
    *this_frame_underlay_occlusion =
        gfx::ToEnclosingRect(occlusion_bounding_box);
  }

  // Propagate the punched holes up the chain of render passes. Punch through
  // rects are in quad target (child render pass) space, and are transformed to
  // RPDQ target (parent render pass) in ProcessRenderPassDrawQuad().
  pass_punch_through_rects_[render_pass->id].push_back(
      gfx::ToEnclosingRect(ClippedQuadRectangle(*it)));

  return true;
}

}  // namespace viz
