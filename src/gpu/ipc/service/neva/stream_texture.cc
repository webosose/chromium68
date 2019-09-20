// Copyright (c) 2018 LG Electronics, Inc.
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

#include "gpu/ipc/service/neva/stream_texture.h"

#include "gpu/command_buffer/service/context_group.h"
#include "gpu/command_buffer/service/context_state.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "gpu/command_buffer/service/texture_manager.h"
#include "gpu/ipc/common/gpu_messages.h"
#include "gpu/ipc/service/gpu_channel.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_helper.h"
#include "ui/gl/scoped_binders.h"
#include "ui/gl/scoped_make_current.h"

namespace gpu {

using gles2::ContextGroup;
using gles2::GLES2Decoder;
using gles2::TextureManager;
using gles2::TextureRef;

// static
bool StreamTexture::Create(CommandBufferStub* owner_stub,
                           int32_t client_texture_id,
                           int32_t stream_id) {
  DecoderContext* decoder = owner_stub->decoder_context();
  TextureManager* texture_manager =
      decoder->GetContextGroup()->texture_manager();
  TextureRef* texture = texture_manager->GetTexture(client_texture_id);
  if (!texture || !gfx::VideoTexture::IsSupported()) {
    return false;
  } else {
    scoped_refptr<gpu::gles2::GLStreamTextureImage> gl_image(
        new StreamTexture(owner_stub, stream_id, client_texture_id));
    gfx::Size size = gl_image->GetSize();
    texture_manager->SetTarget(texture, GL_TEXTURE_EXTERNAL_OES);
    texture_manager->SetLevelInfo(texture, GL_TEXTURE_EXTERNAL_OES, 0, GL_RGBA,
                                  size.width(), size.height(), 1, 0, GL_RGBA,
                                  GL_UNSIGNED_BYTE, gfx::Rect(size));
    texture_manager->SetLevelStreamTextureImage(
        texture, GL_TEXTURE_EXTERNAL_OES, 0, gl_image.get(),
        gles2::Texture::UNBOUND, 0);
    return true;
  }
}

StreamTexture::StreamTexture(CommandBufferStub* owner_stub,
                             int32_t route_id,
                             int32_t texture_id)
    : video_texture_(gfx::VideoTexture::Create(texture_id)),
      active_region_(0, 0, 0, 0),
      has_pending_frame_(false),
      owner_stub_(owner_stub),
      route_id_(route_id),
      has_listener_(false),
      texture_id_(texture_id),
      weak_factory_(this) {
  owner_stub->AddDestructionObserver(this);
  memset(current_matrix_, 0, sizeof(current_matrix_));
  owner_stub->channel()->AddRoute(route_id, owner_stub->sequence_id(), this);
  video_texture_->SetFrameAvailableCallback(base::Bind(
      &StreamTexture::OnFrameAvailable, weak_factory_.GetWeakPtr()));
}

StreamTexture::~StreamTexture() {
  if (owner_stub_) {
    owner_stub_->RemoveDestructionObserver(this);
    owner_stub_->channel()->RemoveRoute(route_id_);
  }
}

// gpu::gles2::GLStreamTextureMatrix implementation
void StreamTexture::GetTextureMatrix(float xform[16]) {
  if (video_texture_) {
    video_texture_->GetTransformMatrix(current_matrix_);
  }
  DCHECK(sizeof(current_matrix_) == 16 * sizeof(float));
  memcpy(xform, current_matrix_, sizeof(current_matrix_));
}

void StreamTexture::OnWillDestroyStub(bool have_context) {
  owner_stub_->RemoveDestructionObserver(this);
  owner_stub_->channel()->RemoveRoute(route_id_);
  owner_stub_ = NULL;
}

std::unique_ptr<ui::ScopedMakeCurrent> StreamTexture::MakeStubCurrent() {
  std::unique_ptr<ui::ScopedMakeCurrent> scoped_make_current;
  bool needs_make_current =
      !owner_stub_->decoder_context()->GetGLContext()->IsCurrent(NULL);
  if (needs_make_current) {
    scoped_make_current.reset(new ui::ScopedMakeCurrent(
        owner_stub_->decoder_context()->GetGLContext(), owner_stub_->surface()));
  }
  return scoped_make_current;
}

void StreamTexture::UpdateTexImage() {
  if (!owner_stub_ || !video_texture_.get() || !has_pending_frame_)
    return;

  std::unique_ptr<ui::ScopedMakeCurrent> scoped_make_current(MakeStubCurrent());

  video_texture_->UpdateTexImage();

  TextureManager* texture_manager =
      owner_stub_->decoder_context()->GetContextGroup()->texture_manager();
  if (texture_manager) {
    gles2::TextureRef* texture = texture_manager->GetTexture(texture_id_);
    if (texture)
      texture->texture()->SetVTGStreamTextureServiceId(
          video_texture_->texture_id());
  }

  has_pending_frame_ = false;

  if (scoped_make_current.get()) {
    // UpdateTexImage() implies glBindTexture().
    // The cmd decoder takes care of restoring the binding for this GLImage as
    // far as the current context is concerned, but if we temporarily change
    // it, we have to keep the state intact in *that* context also.
    const gpu::gles2::ContextState* state =
        owner_stub_->decoder_context()->GetContextState();
    const gpu::gles2::TextureUnit& active_unit =
        state->texture_units[state->active_texture_unit];
    glBindTexture(GL_TEXTURE_EXTERNAL_OES,
                  active_unit.bound_texture_external_oes.get()
                      ? active_unit.bound_texture_external_oes->service_id()
                      : 0);
  }
}

bool StreamTexture::CopyTexImage(unsigned target) {
  if (target != GL_TEXTURE_EXTERNAL_OES)
    return false;

  if (!owner_stub_ || !video_texture_.get())
    return true;

  GLint texture_id;
  glGetIntegerv(GL_TEXTURE_BINDING_EXTERNAL_OES, &texture_id);

  // The following code only works if we're being asked to copy into
  // |texture_id_|. Copying into a same texture is not supported.
  // On some devices GL_TEXTURE_BINDING_EXTERNAL_OES is not supported as
  // glGetIntegerv() parameter. In this case the value of |texture_id| will be
  // zero and we assume that it is properly bound to |texture_id_|.
  if (texture_id > 0 &&  video_texture_->texture_id() > 0 &&
      static_cast<unsigned>(texture_id) != video_texture_->texture_id())
    return false;

  UpdateTexImage();

  TextureManager* texture_manager =
      owner_stub_->decoder_context()->GetContextGroup()->texture_manager();
  gles2::TextureRef* ref =
      texture_manager->GetTexture(texture_id_);
  if (ref && ref->texture()) {
    // By setting image state to UNBOUND instead of COPIED we ensure that
    // CopyTexImage() is called each time the surface texture is used for
    // drawing.
    ref->texture()->SetLevelStreamTextureImage(GL_TEXTURE_EXTERNAL_OES, 0, this,
                                        gles2::Texture::UNBOUND, video_texture_->texture_id());
  }

  return true;
}

void StreamTexture::OnFrameAvailable() {
  has_pending_frame_ = true;
  if (has_listener_ && owner_stub_) {
    owner_stub_->channel()->Send(
        new GpuStreamTextureMsg_FrameAvailable(route_id_));
  }
}

gfx::Size StreamTexture::GetSize() {
  return active_region_.size();
}

unsigned StreamTexture::GetInternalFormat() {
  return GL_RGBA;
}

bool StreamTexture::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(StreamTexture, message)
    IPC_MESSAGE_HANDLER(GpuStreamTextureMsg_StartListening, OnStartListening)
    IPC_MESSAGE_HANDLER(GpuStreamTextureMsg_SetActiveRegion, OnSetActiveRegion)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  DCHECK(handled);
  return handled;
}

void StreamTexture::OnStartListening() {
  DCHECK(!has_listener_);
  has_listener_ = true;
}

void StreamTexture::OnSetActiveRegion(const gfx::Rect& active_region) {
  active_region_ = active_region;
  video_texture_->SetActiveRegion(active_region);
}

bool StreamTexture::BindTexImage(unsigned target) {
  NOTREACHED();
  return false;
}

void StreamTexture::ReleaseTexImage(unsigned target) {
  NOTREACHED();
}

bool StreamTexture::CopyTexSubImage(unsigned target,
                                    const gfx::Point& offset,
                                    const gfx::Rect& rect) {
  return false;
}

bool StreamTexture::ScheduleOverlayPlane(gfx::AcceleratedWidget widget,
                                         int z_order,
                                         gfx::OverlayTransform transform,
                                         const gfx::Rect& bounds_rect,
                                         const gfx::RectF& crop_rect,
                                         bool enable_blend,
                                         std::unique_ptr<gfx::GpuFence> gpu_fence) {
  NOTREACHED();
  return false;
}

void StreamTexture::OnMemoryDump(base::trace_event::ProcessMemoryDump* pmd,
                                 uint64_t process_tracing_id,
                                 const std::string& dump_name) {
  // TODO(ericrk): Add OnMemoryDump for GLImages. crbug.com/514914
}

}  // namespace gpu
