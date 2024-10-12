// Copyright (c) 2024 averne
//
// This file is part of Fizeau.
//
// Fizeau is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Fizeau is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Fizeau.  If not, see <http://www.gnu.org/licenses/>.

#include <cstring>
#include <imgui.h>
#include <switch.h>
#include <deko3d.hpp>
#include <common.hpp>

#include <glm/mat4x4.hpp>

#include "imgui_deko3d.h"
#include "imgui_nx.h"

#include "gfx.hpp"

namespace im = ImGui;

namespace fz::gfx {

namespace {

constexpr auto MAX_SAMPLERS = 4;
constexpr auto MAX_IMAGES   = 4;

constexpr auto FB_NUM       = 2u;

constexpr auto CMDBUF_SIZE  = 1024 * 1024;

unsigned s_width  = 1920;
unsigned s_height = 1080;

dk::UniqueDevice       s_device;

dk::UniqueMemBlock     s_depthMemBlock;
dk::Image              s_depthBuffer;

dk::UniqueMemBlock     s_fbMemBlock;
dk::Image              s_frameBuffers[FB_NUM];

dk::UniqueMemBlock     s_cmdMemBlock;
dk::UniqueCmdBuf       s_cmdBuf;
dk::Fence              s_cmdBufFences[FB_NUM];

dk::UniqueMemBlock     s_descriptorMemBlock;
dk::SamplerDescriptor *s_samplerDescriptors = nullptr;
dk::ImageDescriptor   *s_imageDescriptors   = nullptr;

dk::UniqueMemBlock     s_codeMemBlock;
dk::UniqueMemBlock     s_uniformMemBlock;
dk::Shader             s_previewShr;

dk::UniqueQueue        s_queue;
dk::UniqueSwapchain    s_swapchain;

/* Generated from:
#version 460

layout (local_size_x = 8, local_size_y = 8) in;

layout (std140, binding = 0) uniform UBO {
    mat4 colormatrix;
} p;

layout(rgba8, binding = 0) uniform readonly  image2D img_input;
layout(rgba8, binding = 1) uniform writeonly image2D img_output;

void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID);
    if (pos.x >= 500 || pos.y >= 500)
        return;

    imageStore(img_output, pos, imageLoad(img_input, pos) * p.colormatrix);
}
*/

std::uint8_t preview_shr_ctrl[] = {
    0x44, 0x4b, 0x53, 0x48, 0x18, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
    0x18, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

std::uint8_t preview_shr_code[] = {
    0xed, 0x07, 0x20, 0xe0, 0x00, 0xbc, 0x1c, 0x00, 0x00, 0x00, 0x80, 0x1a, 0x00, 0x00, 0x70, 0xe2,
    0x00, 0x00, 0x57, 0x02, 0x00, 0x00, 0xc8, 0xf0, 0x01, 0x00, 0x07, 0x02, 0x00, 0x00, 0xc8, 0xf0,
    0xe6, 0x17, 0x00, 0xfe, 0x01, 0x04, 0x1c, 0x00, 0x02, 0x01, 0x07, 0x00, 0x01, 0x00, 0x00, 0x38,
    0x02, 0x00, 0x27, 0x00, 0x80, 0x01, 0x18, 0x5c, 0x00, 0x00, 0x67, 0x02, 0x00, 0x00, 0xc8, 0xf0,
    0xee, 0x07, 0x20, 0xfc, 0x01, 0xb4, 0x1f, 0x00, 0x01, 0x01, 0x07, 0xa1, 0x00, 0x00, 0x00, 0x38,
    0x03, 0x00, 0x17, 0x00, 0x80, 0x01, 0x18, 0x5c, 0x07, 0x02, 0x47, 0x1f, 0x80, 0x03, 0x6d, 0x36,
    0xed, 0x07, 0xa0, 0xfd, 0x3f, 0x84, 0xff, 0x07, 0x07, 0x03, 0x47, 0x1f, 0x00, 0x20, 0x6d, 0x36,
    0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xe3, 0x00, 0x00, 0x27, 0x00, 0x80, 0x07, 0x98, 0x5c,
    0xe6, 0x07, 0xe0, 0xe1, 0x00, 0x3c, 0x3c, 0x00, 0x01, 0x00, 0x37, 0x00, 0x80, 0x07, 0x98, 0x5c,
    0x00, 0x00, 0x47, 0x00, 0x86, 0x04, 0x18, 0xeb, 0x01, 0x02, 0x07, 0x00, 0x00, 0x00, 0xb8, 0x5c,
    0xf0, 0x0f, 0xe0, 0xe1, 0x00, 0xc0, 0x3f, 0x00, 0x01, 0x01, 0x17, 0x08, 0x08, 0xb8, 0x03, 0x1e,
    0x04, 0x02, 0x07, 0x00, 0x00, 0x02, 0xb8, 0x5c, 0x07, 0x04, 0x17, 0x08, 0x08, 0xb8, 0x03, 0x1e,
    0x0f, 0x07, 0x00, 0xfe, 0x01, 0x3c, 0x1c, 0x00, 0x04, 0x02, 0x07, 0x00, 0x00, 0x04, 0xb8, 0x5c,
    0x08, 0x04, 0x17, 0x08, 0x08, 0xb8, 0x03, 0x1e, 0x00, 0x02, 0x07, 0x00, 0x00, 0x06, 0xb8, 0x5c,
    0xe1, 0x0f, 0xc0, 0xfc, 0x00, 0x98, 0x1f, 0x00, 0x00, 0x00, 0x17, 0x08, 0x08, 0xb8, 0x03, 0x1e,
    0x04, 0x01, 0x07, 0x00, 0x08, 0x00, 0x68, 0x4c, 0x04, 0x07, 0x17, 0x00, 0x08, 0x02, 0x80, 0x49,
    0xe6, 0x07, 0x20, 0xfc, 0x00, 0x98, 0x1f, 0x00, 0x04, 0x08, 0x27, 0x00, 0x08, 0x02, 0x80, 0x49,
    0x04, 0x00, 0x37, 0x00, 0x08, 0x02, 0x80, 0x49, 0x05, 0x01, 0x47, 0x00, 0x08, 0x00, 0x68, 0x4c,
    0xe6, 0x07, 0xc0, 0xfc, 0x00, 0x84, 0x1f, 0x00, 0x05, 0x07, 0x57, 0x00, 0x88, 0x02, 0x80, 0x49,
    0x05, 0x08, 0x67, 0x00, 0x88, 0x02, 0x80, 0x49, 0x05, 0x00, 0x77, 0x00, 0x88, 0x02, 0x80, 0x49,
    0xe6, 0x07, 0xc0, 0xfc, 0x00, 0x98, 0x1f, 0x00, 0x06, 0x01, 0x87, 0x00, 0x08, 0x00, 0x68, 0x4c,
    0x06, 0x07, 0x97, 0x00, 0x08, 0x03, 0x80, 0x49, 0x06, 0x08, 0xa7, 0x00, 0x08, 0x03, 0x80, 0x49,
    0xe1, 0x07, 0xc0, 0xfc, 0x00, 0x98, 0x1f, 0x00, 0x06, 0x00, 0xb7, 0x00, 0x08, 0x03, 0x80, 0x49,
    0x01, 0x01, 0xc7, 0x00, 0x08, 0x00, 0x68, 0x4c, 0x01, 0x07, 0xd7, 0x00, 0x88, 0x00, 0x80, 0x49,
    0xe6, 0x07, 0x20, 0xfc, 0x00, 0x84, 0x1f, 0x00, 0x01, 0x08, 0xe7, 0x00, 0x88, 0x00, 0x80, 0x49,
    0x07, 0x00, 0xf7, 0x00, 0x88, 0x00, 0x80, 0x49, 0x00, 0x00, 0x27, 0x00, 0x80, 0x07, 0x98, 0x5c,
    0xe6, 0x07, 0x20, 0x1c, 0x00, 0xbc, 0xff, 0x07, 0x01, 0x00, 0x37, 0x00, 0x80, 0x07, 0x98, 0x5c,
    0x04, 0x00, 0xf7, 0x00, 0x96, 0x04, 0x28, 0xeb, 0x0f, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0xe3,
    0xff, 0x07, 0x00, 0xfc, 0x00, 0x80, 0x1f, 0x00, 0x0f, 0x00, 0x07, 0xff, 0xff, 0x0f, 0x40, 0xe2,
    0x00, 0x0f, 0x07, 0x00, 0x00, 0x00, 0xb0, 0x50, 0x00, 0x0f, 0x07, 0x00, 0x00, 0x00, 0xb0, 0x50,
    0xe0, 0x07, 0x00, 0xfc, 0x00, 0x80, 0x1f, 0x00, 0x00, 0x0f, 0x07, 0x00, 0x00, 0x00, 0xb0, 0x50,
    0x00, 0x0f, 0x07, 0x00, 0x00, 0x00, 0xb0, 0x50, 0x00, 0x0f, 0x07, 0x00, 0x00, 0x00, 0xb0, 0x50,
};

void rebuildSwapchain(unsigned const width_, unsigned const height_) {
    // destroy old swapchain
    s_swapchain = nullptr;

    // create new depth buffer image layout
    dk::ImageLayout depthLayout;
    dk::ImageLayoutMaker{s_device}
        .setFlags(DkImageFlags_UsageRender | DkImageFlags_HwCompression)
        .setFormat(DkImageFormat_Z24S8)
        .setDimensions(width_, height_)
        .initialize(depthLayout);

    auto const depthAlign = depthLayout.getAlignment();
    auto const depthSize  = depthLayout.getSize();

    // create depth buffer memblock
    if (!s_depthMemBlock) {
        s_depthMemBlock = dk::MemBlockMaker{s_device,
            im::deko3d::align(
                depthSize, std::max<unsigned> (depthAlign, DK_MEMBLOCK_ALIGNMENT))}
                              .setFlags(DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image)
                              .create();
    }

    s_depthBuffer.initialize(depthLayout, s_depthMemBlock, 0);

    // create framebuffer image layout
    dk::ImageLayout fbLayout;
    dk::ImageLayoutMaker{s_device}
        .setFlags(DkImageFlags_UsageRender | DkImageFlags_UsagePresent | DkImageFlags_HwCompression)
        .setFormat(DkImageFormat_RGBA8_Unorm)
        .setDimensions(width_, height_)
        .initialize(fbLayout);

    auto const fbAlign = fbLayout.getAlignment();
    auto const fbSize  = fbLayout.getSize();

    // create framebuffer memblock
    if (!s_fbMemBlock) {
        s_fbMemBlock = dk::MemBlockMaker{s_device,
            im::deko3d::align(
                FB_NUM * fbSize, std::max<unsigned> (fbAlign, DK_MEMBLOCK_ALIGNMENT))}
                           .setFlags(DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image)
                           .create();
    }

    // initialize swapchain images
    std::array<DkImage const *, FB_NUM> swapchainImages;
    for (unsigned i = 0; i < FB_NUM; ++i) {
        swapchainImages[i] = &s_frameBuffers[i];
        s_frameBuffers[i].initialize(fbLayout, s_fbMemBlock, i * fbSize);
    }

    // create swapchain
    s_swapchain = dk::SwapchainMaker{s_device, nwindowGetDefault(), swapchainImages}.create();
}

void deko3dInit() {
    // create deko3d device
    s_device = dk::DeviceMaker{}.create();

    // create queue
    s_queue = dk::QueueMaker{s_device}
        .setFlags(DkQueueFlags_Graphics | DkQueueFlags_Compute)
        .create();

    // initialize swapchain with maximum resolution
    rebuildSwapchain(1920, 1080);

    // create command buffer memblock
    s_cmdMemBlock =
        dk::MemBlockMaker{s_device, im::deko3d::align(CMDBUF_SIZE * FB_NUM, DK_MEMBLOCK_ALIGNMENT)}
            .setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached)
            .create();

    // create command buffer
    s_cmdBuf = dk::CmdBufMaker{s_device}.create();
    s_cmdBuf.addMemory(s_cmdMemBlock, 0, CMDBUF_SIZE);

    // create shader uniforms memblock
    s_uniformMemBlock = dk::MemBlockMaker{s_device, im::deko3d::align(sizeof(glm::mat4), DK_MEMBLOCK_ALIGNMENT)}
        .setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached)
        .create();

    // create shader memblock
    s_codeMemBlock = dk::MemBlockMaker{s_device, im::deko3d::align(sizeof(preview_shr_code), DK_MEMBLOCK_ALIGNMENT)}
        .setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached | DkMemBlockFlags_Code)
        .create();

    std::memcpy(s_codeMemBlock.getCpuAddr(), preview_shr_code, sizeof(preview_shr_code));

    dk::ShaderMaker{s_codeMemBlock, 0}
        .setControl(preview_shr_ctrl)
        .initialize(s_previewShr);

    // create image/sampler memblock
    static_assert(sizeof(dk::ImageDescriptor)   == DK_IMAGE_DESCRIPTOR_ALIGNMENT);
    static_assert(sizeof(dk::SamplerDescriptor) == DK_SAMPLER_DESCRIPTOR_ALIGNMENT);
    static_assert(DK_IMAGE_DESCRIPTOR_ALIGNMENT == DK_SAMPLER_DESCRIPTOR_ALIGNMENT);
    s_descriptorMemBlock = dk::MemBlockMaker{s_device,
        im::deko3d::align(
            (MAX_SAMPLERS + MAX_IMAGES) * sizeof(dk::ImageDescriptor), DK_MEMBLOCK_ALIGNMENT)}
                               .setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached)
                               .create();

    // get cpu address for descriptors
    s_samplerDescriptors =
        static_cast<dk::SamplerDescriptor *> (s_descriptorMemBlock.getCpuAddr());
    s_imageDescriptors =
        reinterpret_cast<dk::ImageDescriptor *> (&s_samplerDescriptors[MAX_SAMPLERS]);

    // bind image/sampler descriptors
    s_cmdBuf.bindSamplerDescriptorSet(s_descriptorMemBlock.getGpuAddr(), MAX_SAMPLERS);
    s_cmdBuf.bindImageDescriptorSet(
        s_descriptorMemBlock.getGpuAddr() + MAX_SAMPLERS * sizeof(dk::SamplerDescriptor),
        MAX_IMAGES);
    s_queue.submitCommands(s_cmdBuf.finishList());
}

void deko3dExit() {
    // clean up all of the deko3d objects
    s_descriptorMemBlock = nullptr;

    s_cmdBuf             = nullptr;
    s_cmdMemBlock        = nullptr;

    s_uniformMemBlock    = nullptr;
    s_codeMemBlock       = nullptr;

    s_queue              = nullptr;
    s_swapchain          = nullptr;
    s_fbMemBlock         = nullptr;
    s_depthMemBlock      = nullptr;
    s_device             = nullptr;
}

} // namespace

bool init() {
    im::CreateContext();
    if (!im::nx::init())
        return false;

    deko3dInit();
    im::deko3d::init(s_device,
        s_queue,
        s_cmdBuf,
        s_samplerDescriptors[0],
        s_imageDescriptors[0],
        dkMakeTextureHandle(0, 0),
        FB_NUM);

    return true;
}

bool loop() {
    if (!appletMainLoop())
        return false;

    auto down = im::nx::newFrame();
    im::NewFrame();

    return !(down & HidNpadButton_Plus);
}

int dequeue() {
    auto &io = im::GetIO();
    if (s_width != io.DisplaySize.x || s_height != io.DisplaySize.y) {
        s_width  = io.DisplaySize.x;
        s_height = io.DisplaySize.y;
        rebuildSwapchain(s_width, s_height);
    }

    // get image from queue
    auto slot = s_queue.acquireImage(s_swapchain);
    s_cmdBuf.clear();
    s_cmdBuf.addMemory(s_cmdMemBlock, slot * CMDBUF_SIZE, CMDBUF_SIZE);
    s_cmdBufFences[slot].wait();

    return slot;
}

void render(int slot) {
    im::Render();

    // bind frame/depth buffers and clear them
    dk::ImageView colorTarget{s_frameBuffers[slot]};
    dk::ImageView depthTarget{s_depthBuffer};
    s_cmdBuf.bindRenderTargets(&colorTarget, &depthTarget);
    s_cmdBuf.setScissors(0, DkScissor{0, 0, s_width, s_height});
    s_cmdBuf.clearColor(0, DkColorMask_RGBA, 0.35f, 0.30f, 0.35f, 1.0f);
    s_cmdBuf.clearDepthStencil(true, 1.0f, 0xFF, 0);

    im::deko3d::render(s_device, s_queue, s_cmdBuf, slot);

    // wait for fragments to be completed before discarding depth/stencil buffer
    s_cmdBuf.barrier(DkBarrier_Fragments, 0);
    s_cmdBuf.signalFence(s_cmdBufFences[slot]);
    s_cmdBuf.discardDepthStencil();

    s_queue.submitCommands(s_cmdBuf.finishList());

    // present image
    s_queue.presentImage(s_swapchain, slot);
}

void render_preview(FizeauSettings &settings, int width, int height, int src_image_id, int dst_image_id) {
    // Calculate initial coefficients
    auto coeffs = filter_matrix(settings.filter);

    // Apply temperature color correction
    ColorMatrix wp = {};
    std::tie(wp[0], wp[4], wp[8]) = whitepoint(settings.temperature);
    coeffs = dot(coeffs, wp);

    // Apply saturation
    coeffs = dot(coeffs, saturation_matrix(settings.saturation));

    // Apply hue rotation
    coeffs = dot(coeffs, hue_matrix(settings.hue));

    auto colormatrix = glm::mat4(
        coeffs[0], coeffs[1], coeffs[2], 0,
        coeffs[3], coeffs[4], coeffs[5], 0,
        coeffs[6], coeffs[7], coeffs[8], 0,
        0,         0,         0,         1
    );

    s_cmdBuf.pushConstants(s_uniformMemBlock.getGpuAddr(), s_uniformMemBlock.getSize(), 0, sizeof(colormatrix), &colormatrix);
    s_cmdBuf.bindUniformBuffer(DkStage_Compute, 0, s_uniformMemBlock.getGpuAddr(), s_uniformMemBlock.getSize());
    s_cmdBuf.bindImages(DkStage_Compute, 0, { (std::uint32_t)src_image_id, (std::uint32_t)dst_image_id });
    s_cmdBuf.bindShaders(DkStageFlag_Compute, &s_previewShr);
    s_cmdBuf.dispatchCompute((width + 7) / 8, (height + 7) / 8, 1);
    s_cmdBuf.barrier(DkBarrier_Primitives, 0);

    s_queue.submitCommands(s_cmdBuf.finishList());
}

void wait() {
    // wait for queue to be idle
    s_queue.waitIdle();
}

void exit() {
    im::nx::exit();

    im::deko3d::exit();
    deko3dExit();
}

static void update_descriptors(dk::Image &image, int sampler_id, int image_id) {
    // initialize image descriptor
    s_imageDescriptors[image_id].initialize(image);

    // initialize sampler descriptor
    s_samplerDescriptors[sampler_id].initialize(
        dk::Sampler{}
            .setFilter(DkFilter_Linear, DkFilter_Linear)
            .setWrapMode(DkWrapMode_ClampToEdge, DkWrapMode_ClampToEdge, DkWrapMode_ClampToEdge));

    s_cmdBuf.barrier(DkBarrier_None, DkInvalidateFlags_Descriptors);
    s_queue.waitIdle();
}

void create_texture(dk::MemBlock &memblk, dk::Image &image, int width, int height, DkImageFormat fmt,
    std::uint32_t sampler_id, std::uint32_t image_id)
{
    dk::ImageLayout layout;
    dk::ImageLayoutMaker{s_device}
        .setFlags(DkImageFlags_UsageRender | DkImageFlags_UsageLoadStore | DkImageFlags_Usage2DEngine | DkImageFlags_HwCompression)
        .setFormat(fmt)
        .setDimensions(width, height)
        .initialize(layout);

    memblk = dk::MemBlockMaker{s_device, im::deko3d::align(layout.getSize(), DK_MEMBLOCK_ALIGNMENT)}
        .setFlags(DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image)
        .create();

    image.initialize(layout, memblk, 0);

    update_descriptors(image, sampler_id, image_id);
}

void register_texture(dk::MemBlock &memblk, dk::Image &image, const nj::Surface &surf,
    std::uint32_t sampler_id, std::uint32_t image_id)
{
    // map data into deko3d
    std::tie(memblk, image) = surf.to_deko3d(s_device, DkImageFlags_Usage2DEngine);

    update_descriptors(image, sampler_id, image_id);
}

} // namespace fz::gfx
