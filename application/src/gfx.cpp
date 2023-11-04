// Copyright (C) 2020 averne
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

#include <imgui.h>
#include <switch.h>
#include <deko3d.hpp>
#include <common.hpp>

#include "imgui_deko3d.h"
#include "imgui_nx.h"

#include "gfx.hpp"

namespace im = ImGui;

namespace fz::gfx {

namespace {

constexpr auto MAX_SAMPLERS = 3;
constexpr auto MAX_IMAGES   = 4;

constexpr auto FB_NUM       = 3u;

constexpr auto CMDBUF_SIZE  = 1024 * 1024;

unsigned s_width  = 1920;
unsigned s_height = 1080;

dk::UniqueDevice       s_device;

dk::UniqueMemBlock     s_depthMemBlock;
dk::Image              s_depthBuffer;

dk::UniqueMemBlock     s_fbMemBlock;
dk::Image              s_frameBuffers[FB_NUM];

dk::UniqueMemBlock     s_cmdMemBlock[FB_NUM];
dk::UniqueCmdBuf       s_cmdBuf[FB_NUM];

dk::UniqueMemBlock     s_descriptorMemBlock;
dk::SamplerDescriptor *s_samplerDescriptors = nullptr;
dk::ImageDescriptor   *s_imageDescriptors   = nullptr;

dk::UniqueQueue        s_queue;
dk::UniqueSwapchain    s_swapchain;

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

    // initialize swapchain with maximum resolution
    rebuildSwapchain(1920, 1080);

    // create memblocks for each image slot
    for (std::size_t i = 0; i < FB_NUM; ++i) {
        // create command buffer memblock
        s_cmdMemBlock[i] =
            dk::MemBlockMaker{s_device, im::deko3d::align(CMDBUF_SIZE, DK_MEMBLOCK_ALIGNMENT)}
                .setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached)
                .create();

        // create command buffer
        s_cmdBuf[i] = dk::CmdBufMaker{s_device}.create();
        s_cmdBuf[i].addMemory(s_cmdMemBlock[i], 0, s_cmdMemBlock[i].getSize());
    }

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

    // create queue
    s_queue = dk::QueueMaker{s_device}.setFlags(DkQueueFlags_Graphics).create();

    auto &cmdBuf = s_cmdBuf[0];

    // bind image/sampler descriptors
    cmdBuf.bindSamplerDescriptorSet(s_descriptorMemBlock.getGpuAddr(), MAX_SAMPLERS);
    cmdBuf.bindImageDescriptorSet(
        s_descriptorMemBlock.getGpuAddr() + MAX_SAMPLERS * sizeof(dk::SamplerDescriptor),
        MAX_IMAGES);
    s_queue.submitCommands(cmdBuf.finishList());
    s_queue.waitIdle();

    cmdBuf.clear();
}

void deko3dExit() {
    // clean up all of the deko3d objects
    s_descriptorMemBlock = nullptr;

    for (unsigned i = 0; i < FB_NUM; ++i) {
        s_cmdBuf[i]      = nullptr;
        s_cmdMemBlock[i] = nullptr;
    }

    s_queue         = nullptr;
    s_swapchain     = nullptr;
    s_fbMemBlock    = nullptr;
    s_depthMemBlock = nullptr;
    s_device        = nullptr;
}

} // namespace

bool init() {
    im::CreateContext();
    if (!im::nx::init())
        return false;

    deko3dInit();
    im::deko3d::init(s_device,
        s_queue,
        s_cmdBuf[0],
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

void render() {
    im::Render();

    auto &io = im::GetIO();

    if (s_width != io.DisplaySize.x || s_height != io.DisplaySize.y) {
        s_width  = io.DisplaySize.x;
        s_height = io.DisplaySize.y;
        rebuildSwapchain(s_width, s_height);
    }

    // get image from queue
    auto const slot = s_queue.acquireImage(s_swapchain);
    auto &cmdBuf    = s_cmdBuf[slot];

    cmdBuf.clear();

    // bind frame/depth buffers and clear them
    dk::ImageView colorTarget{s_frameBuffers[slot]};
    dk::ImageView depthTarget{s_depthBuffer};
    cmdBuf.bindRenderTargets(&colorTarget, &depthTarget);
    cmdBuf.setScissors(0, DkScissor{0, 0, s_width, s_height});
    cmdBuf.clearColor(0, DkColorMask_RGBA, 0.35f, 0.30f, 0.35f, 1.0f);
    cmdBuf.clearDepthStencil(true, 1.0f, 0xFF, 0);
    s_queue.submitCommands(cmdBuf.finishList());

    im::deko3d::render(s_device, s_queue, cmdBuf, slot);

    // wait for fragments to be completed before discarding depth/stencil buffer
    cmdBuf.barrier(DkBarrier_Fragments, 0);
    cmdBuf.discardDepthStencil();

    // present image
    s_queue.presentImage(s_swapchain, slot);
}

void exit() {
    im::nx::exit();

    // wait for queue to be idle
    s_queue.waitIdle();

    im::deko3d::exit();
    deko3dExit();
}

void register_texture(dk::MemBlock memblk, dk::Image &image, const nj::Surface &surf, std::uint32_t sampler_id, std::uint32_t image_id) {
    // upload data to deko3d
    std::tie(memblk, image) = surf.to_deko3d(s_device, 0);

    // initialize image descriptor
    s_imageDescriptors[image_id].initialize(image);

    // initialize sampler descriptor
    s_samplerDescriptors[sampler_id].initialize(
        dk::Sampler{}
            .setFilter(DkFilter_Linear, DkFilter_Linear)
            .setWrapMode(DkWrapMode_ClampToEdge, DkWrapMode_ClampToEdge, DkWrapMode_ClampToEdge));
}

} // namespace fz::gfx
