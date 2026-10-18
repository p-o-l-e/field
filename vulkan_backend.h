#pragma once

/******************************************************************************************************************************
 * Vulkan rendering backend for the Field GUI framework.
 *
 * Replaces the previous OpenGL 3.3 immediate-mode renderer. The CPU-side
 * scene rendering (Frame pixel buffers in 0xRRGGBBAA) is unchanged — this
 * backend only handles GPU presentation of those buffers plus line-strip
 * overlays (oscillograph stream, connection cords).
 *
 * Design notes (performance-critical, see AGENTS.md):
 * - No per-frame heap allocation. All Vulkan objects are created upfront.
 * - Double-buffered frames in flight (MAX_FRAMES_IN_FLIGHT = 2).
 * - Single command pool; command buffers are reset and re-recorded each frame.
 * - Texture uploads go through a single reusable staging buffer.
 * - Push constants for line color avoid per-draw descriptor updates.
 * - Descriptor sets are pre-allocated per layer texture; only the image view
 *   is updated when a texture is (re)created.
 ******************************************************************************************************************************/

/* Vulkan headers must come before GLFW so that VkResult/VkInstance/etc.
 * are available when glfw3.h declares glfwCreateWindowSurface. */
#define VK_USE_PLATFORM_XLIB_KHR 1
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "field.h"

/* Maximum frames simultaneously submitted to the GPU. */
#define VK_MAX_FRAMES_IN_FLIGHT 2

/* Number of layer textures managed by the backend (BG, NG, FG, TL, ST, DO). */
#define RENDERER_LAYER_COUNT 6

/* Maximum vertices for a single line strip draw (oscillograph uses 128). */
#define VK_LINE_MAX_VERTICES 128

/* Maximum line strip draws per frame (stream + pending cord + established cords). */
#define VK_LINE_MAX_DRAWS 64

/* Line VBO size: enough for VK_LINE_MAX_DRAWS draws of VK_LINE_MAX_VERTICES each. */
#define VK_LINE_VBO_SIZE (VK_LINE_MAX_VERTICES * 2 * sizeof(float) * VK_LINE_MAX_DRAWS)

/* Embedded SPIR-V shaders (compiled from shaders/ .vert and .frag at build time).
 * #embed gives char arrays; SPIR-V is 4-byte aligned so we cast to uint32_t*.
 * Note: #embed must be on its own line (C23 preprocessor directive). */
static const char layer_vert_spv_data[] = {
    #embed "../shaders/layer.vert.spv"
};
static const uint32_t* layer_vert_spv = (const uint32_t*)layer_vert_spv_data;
static const size_t  layer_vert_spv_len = sizeof(layer_vert_spv_data);

static const char layer_frag_spv_data[] = {
    #embed "../shaders/layer.frag.spv"
};
static const uint32_t* layer_frag_spv = (const uint32_t*)layer_frag_spv_data;
static const size_t  layer_frag_spv_len = sizeof(layer_frag_spv_data);

static const char line_vert_spv_data[] = {
    #embed "../shaders/line.vert.spv"
};
static const uint32_t* line_vert_spv = (const uint32_t*)line_vert_spv_data;
static const size_t  line_vert_spv_len = sizeof(line_vert_spv_data);

static const char line_frag_spv_data[] = {
    #embed "../shaders/line.frag.spv"
};
static const uint32_t* line_frag_spv = (const uint32_t*)line_frag_spv_data;
static const size_t  line_frag_spv_len = sizeof(line_frag_spv_data);

/******************************************************************************************************************************
 * Vulkan context — all state for a single window.
 ******************************************************************************************************************************/

typedef struct {
    /* Per-layer texture state */
    VkImage        image;
    VkDeviceMemory memory;
    VkImageView    view;
    VkSampler      sampler;
    VkDescriptorSet descriptor_set;   /* bound to layer pipeline layout */
    bool           valid;             /* texture has been created */
} VkLayerTexture;

typedef struct {
    /* Core objects */
    VkInstance       instance;
    VkSurfaceKHR     surface;
    VkPhysicalDevice physical_device;
    VkDevice         device;
    VkQueue          graphics_queue;
    VkQueue          present_queue;
    uint32_t         graphics_family;
    uint32_t         present_family;

    /* Swapchain */
    VkSwapchainKHR   swapchain;
    VkFormat         swapchain_format;
    VkExtent2D       swapchain_extent;
    uint32_t         swapchain_image_count;
    VkImage*         swapchain_images;       /* owned by swapchain */
    VkImageView*     swapchain_views;
    VkFramebuffer*   framebuffers;

    /* MSAA color attachment (one image per swapchain image); the render pass
     * resolves it into the swapchain image. Antialiases line primitives. */
    VkSampleCountFlagBits msaa_samples;
    VkImage*         msaa_images;
    VkDeviceMemory*  msaa_memories;
    VkImageView*     msaa_views;

    /* Render pass */
    VkRenderPass     render_pass;

    /* Pipelines */
    VkPipelineLayout layer_pipeline_layout;
    VkPipeline       layer_pipeline;
    VkPipelineLayout line_pipeline_layout;
    VkPipeline       line_pipeline;

    /* Descriptor pool (for per-layer texture descriptor sets) */
    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout layer_descriptor_set_layout;

    /* Layer textures */
    VkLayerTexture   layers[RENDERER_LAYER_COUNT];

    /* Full-screen quad vertex buffer (pos.xy + uv.xy interleaved, 6 verts) */
    VkBuffer         quad_vbo;
    VkDeviceMemory   quad_vbo_memory;

    /* Dynamic line vertex buffer (2 floats per vertex, pre-allocated).
     * Multiple line draws per frame each get their own slot via line_vbo_offset
     * so the CPU doesn't overwrite vertex data before the GPU reads it. */
    VkBuffer         line_vbo;
    VkDeviceMemory   line_vbo_memory;
    VkDeviceSize     line_vbo_offset;         /* reset to 0 each frame */

    /* Staging buffer for texture uploads (reused every frame) */
    VkBuffer         staging_buffer;
    VkDeviceMemory   staging_memory;
    void*            staging_mapped;          /* persistently mapped */
    VkDeviceSize     staging_size;

    /* Command pool and per-frame command buffers */
    VkCommandPool    command_pool;
    VkCommandBuffer  command_buffers[VK_MAX_FRAMES_IN_FLIGHT];

    /* Per-frame synchronization */
    VkSemaphore      image_available[VK_MAX_FRAMES_IN_FLIGHT];
    VkSemaphore      render_finished[VK_MAX_FRAMES_IN_FLIGHT];
    VkFence          in_flight_fences[VK_MAX_FRAMES_IN_FLIGHT];

    /* Frame tracking */
    uint32_t         current_frame;          /* 0..MAX_FRAMES_IN_FLIGHT-1 */
    uint32_t         width;
    uint32_t         height;

    /* Window handle (not owned) */
    GLFWwindow*      window;
} RendererContext;

/******************************************************************************************************************************
 * Helpers
 ******************************************************************************************************************************/

[[nodiscard]] static uint32_t vk_find_memory_type(VkPhysicalDevice phys, uint32_t type_filter,
                                     VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(phys, &mem_props);
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((type_filter & (1u << i)) &&
            (mem_props.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    fprintf(stderr, "[Vulkan] Failed to find suitable memory type\n");
    return UINT32_MAX;
}

[[nodiscard]] static VkShaderModule vk_create_shader_module(VkDevice device, const uint32_t* restrict code, size_t len)
{
    VkShaderModuleCreateInfo info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = len,
        .pCode    = code,
    };
    VkShaderModule module;
    if (vkCreateShaderModule(device, &info, nullptr, &module) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to create shader module\n");
        return VK_NULL_HANDLE;
    }
    return module;
}

/* Find a queue family that supports both graphics and present. */
[[nodiscard]] static bool vk_find_queue_families(VkPhysicalDevice phys, VkSurfaceKHR surface,
                                    uint32_t* restrict gfx, uint32_t* restrict present)
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phys, &count, nullptr);
    VkQueueFamilyProperties* props = malloc(count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(phys, &count, props);

    *gfx = UINT32_MAX;
    *present = UINT32_MAX;
    for (uint32_t i = 0; i < count; ++i) {
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            if (*gfx == UINT32_MAX) *gfx = i;
            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(phys, i, surface, &present_support);
            if (present_support && *present == UINT32_MAX) *present = i;
        }
    }
    free(props);
    return *gfx != UINT32_MAX && *present != UINT32_MAX;
}

/* Check swapchain support and pick format/present mode. */
[[nodiscard]] static bool vk_query_swapchain_support(VkPhysicalDevice phys, VkSurfaceKHR surface,
                                        VkSurfaceFormatKHR* restrict format, VkPresentModeKHR* restrict mode,
                                        VkExtent2D* extent)
{
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, surface, &caps);

    uint32_t fmt_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &fmt_count, nullptr);
    if (fmt_count == 0) return false;
    VkSurfaceFormatKHR* formats = malloc(fmt_count * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &fmt_count, formats);

    /* Prefer UNORM (not sRGB) — our palette colors are already sRGB-encoded
     * hex values. An sRGB swapchain would apply a linear→sRGB transfer on
     * write, gamma-encoding the colors a second time and bleaching them.
     * UNORM writes shader output directly, matching the old OpenGL behavior. */
    *format = formats[0];
    for (uint32_t i = 0; i < fmt_count; ++i) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            *format = formats[i];
            break;
        }
    }
    free(formats);

    uint32_t mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &mode_count, nullptr);
    if (mode_count == 0) return false;
    VkPresentModeKHR* modes = malloc(mode_count * sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &mode_count, modes);

    *mode = VK_PRESENT_MODE_FIFO_KHR; /* always available, vsync */
    for (uint32_t i = 0; i < mode_count; ++i) {
        if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            *mode = modes[i];
            break;
        }
    }
    free(modes);

    if (caps.currentExtent.width != UINT32_MAX) {
        *extent = caps.currentExtent;
    } else {
        extent->width  = 1280;
        extent->height = 720;
    }
    return true;
}

/******************************************************************************************************************************
 * Buffer creation helper
 ******************************************************************************************************************************/

static bool vk_create_buffer(VkDevice device, VkPhysicalDevice phys,
                              VkDeviceSize size, VkBufferUsageFlags usage,
                              VkMemoryPropertyFlags props,
                              VkBuffer* restrict buffer, VkDeviceMemory* restrict memory)
{
    VkBufferCreateInfo buf_info = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,
        .usage       = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    if (vkCreateBuffer(device, &buf_info, nullptr, buffer) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to create buffer\n");
        return false;
    }

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device, *buffer, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = mem_reqs.size,
        .memoryTypeIndex = vk_find_memory_type(phys, mem_reqs.memoryTypeBits, props),
    };
    if (alloc_info.memoryTypeIndex == UINT32_MAX) return false;

    if (vkAllocateMemory(device, &alloc_info, nullptr, memory) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to allocate buffer memory\n");
        return false;
    }
    vkBindBufferMemory(device, *buffer, *memory, 0);
    return true;
}

/******************************************************************************************************************************
 * Layer texture creation — one VkImage per Frame layer.
 ******************************************************************************************************************************/

static bool vk_create_layer_texture(RendererContext* restrict ctx, VkLayerTexture* restrict lt,
                                     uint32_t width, uint32_t height)
{
    /* Destroy old texture if it exists */
    if (lt->valid) {
        vkDeviceWaitIdle(ctx->device);
        if (lt->sampler)    vkDestroySampler(ctx->device, lt->sampler, nullptr);
        if (lt->view)       vkDestroyImageView(ctx->device, lt->view, nullptr);
        if (lt->image)      vkDestroyImage(ctx->device, lt->image, nullptr);
        if (lt->memory)     vkFreeMemory(ctx->device, lt->memory, nullptr);
        lt->valid = false;
    }

    /* Create image */
    VkImageCreateInfo img_info = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = VK_FORMAT_R8G8B8A8_UNORM,
        .extent        = { width, height, 1 },
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    if (vkCreateImage(ctx->device, &img_info, nullptr, &lt->image) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to create layer image\n");
        return false;
    }

    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(ctx->device, lt->image, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = mem_reqs.size,
        .memoryTypeIndex = vk_find_memory_type(ctx->physical_device,
                                                mem_reqs.memoryTypeBits,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };
    if (alloc_info.memoryTypeIndex == UINT32_MAX) return false;
    if (vkAllocateMemory(ctx->device, &alloc_info, nullptr, &lt->memory) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to allocate layer image memory\n");
        return false;
    }
    vkBindImageMemory(ctx->device, lt->image, lt->memory, 0);

    /* Create image view */
    VkImageViewCreateInfo view_info = {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image    = lt->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = VK_FORMAT_R8G8B8A8_UNORM,
        .subresourceRange = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
    };
    if (vkCreateImageView(ctx->device, &view_info, nullptr, &lt->view) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to create layer image view\n");
        return false;
    }

    /* Create sampler */
    VkSamplerCreateInfo sampler_info = {
        .sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter    = VK_FILTER_LINEAR,
        .minFilter    = VK_FILTER_LINEAR,
        .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .maxLod       = 1.0f,
    };
    if (vkCreateSampler(ctx->device, &sampler_info, nullptr, &lt->sampler) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to create layer sampler\n");
        return false;
    }

    /* Update descriptor set to point at this texture */
    VkDescriptorImageInfo img_desc = {
        .sampler     = lt->sampler,
        .imageView   = lt->view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkWriteDescriptorSet write = {
        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet           = lt->descriptor_set,
        .dstBinding       = 0,
        .dstArrayElement  = 0,
        .descriptorCount  = 1,
        .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo       = &img_desc,
    };
    vkUpdateDescriptorSets(ctx->device, 1, &write, 0, nullptr);

    lt->valid = true;
    return true;
}

/******************************************************************************************************************************
 * Upload Frame pixel data to a layer texture via staging buffer + copy cmd.
 * This is called inside a one-time command buffer during initialization,
 * and per-frame via the main command buffer for updates.
 ******************************************************************************************************************************/

static void renderer_upload_layer(RendererContext* restrict ctx, int layer_index,
                                    const uint32_t* restrict pixels, uint32_t width, uint32_t height)
{
    VkLayerTexture* lt = &ctx->layers[layer_index];
    VkDeviceSize size = (VkDeviceSize)width * height * sizeof(uint32_t);

    /* The staging buffer holds one full-layer slot per layer texture, per
     * frame in flight — per-frame updates write each layer into its own slot
     * (strided by the full layer size) so copies in one command buffer don't
     * clobber each other, and so a frame's CPU-side memcpy can't overwrite
     * data the previous frame's copy is still reading on the GPU (each frame
     * slot is only reused after its in-flight fence is waited on in
     * renderer_begin_frame). Growing it here (once, at init) also covers the
     * per-frame path. */
    VkDeviceSize alloc = size * RENDERER_LAYER_COUNT * VK_MAX_FRAMES_IN_FLIGHT;
    if (alloc > ctx->staging_size) {
        if (ctx->staging_buffer) {
            vkDestroyBuffer(ctx->device, ctx->staging_buffer, nullptr);
            vkFreeMemory(ctx->device, ctx->staging_memory, nullptr);
        }
        vk_create_buffer(ctx->device, ctx->physical_device, alloc,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &ctx->staging_buffer, &ctx->staging_memory);
        ctx->staging_size = alloc;
        vkMapMemory(ctx->device, ctx->staging_memory, 0, alloc, 0, &ctx->staging_mapped);
    }

    /* Copy pixels into staging buffer */
    memcpy(ctx->staging_mapped, pixels, size);

    /* Record a one-time command buffer for layout transition + copy */
    VkCommandBufferAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = ctx->command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(ctx->device, &alloc_info, &cmd);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(cmd, &begin_info);

    /* Transition image to TRANSFER_DST */
    VkImageMemoryBarrier barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask       = 0,
        .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = lt->image,
        .subresourceRange    = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
    };
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    /* Copy staging buffer to image */
    VkBufferImageCopy region = {
        .bufferOffset      = 0,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource  = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel       = 0,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = { width, height, 1 },
    };
    vkCmdCopyBufferToImage(cmd, ctx->staging_buffer, lt->image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    /* Transition image to SHADER_READ_ONLY */
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &cmd,
    };
    vkQueueSubmit(ctx->graphics_queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx->graphics_queue);

    vkFreeCommandBuffers(ctx->device, ctx->command_pool, 1, &cmd);
}

/* Re-upload pixels to an existing texture (per-frame update path).
 * Records into an already-begun command buffer, OUTSIDE the render pass.
 * Only the (x, y, w, h) sub-region is copied (dirty-rect upload): source
 * pixels are read as pixels[(y + row) * width + x + col] and packed tightly
 * into the staging slot (bufferRowLength = 0 in the copy region below).
 * `buffer_offset` is the byte offset into the staging buffer for this layer's
 * pixels — each layer gets its own slot so multiple updates in one command
 * buffer don't overwrite each other before the GPU reads them. */
static void vk_update_layer_texture(VkCommandBuffer cmd, RendererContext* ctx,
                                     VkLayerTexture* lt,
                                     const uint32_t* pixels, uint32_t width,
                                     uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                                     VkDeviceSize buffer_offset)
{
    uint32_t* dst = (uint32_t*)((char*)ctx->staging_mapped + buffer_offset);
    for (uint32_t row = 0; row < h; ++row)
        memcpy(dst + (size_t)row * w, pixels + (size_t)(y + row) * width + x,
               (size_t)w * sizeof(uint32_t));

    /* Transition to TRANSFER_DST */
    VkImageMemoryBarrier barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask       = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = lt->image,
        .subresourceRange    = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
    };
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region = {
        .bufferOffset      = buffer_offset,
        .bufferRowLength   = 0, /* tightly packed: row length = imageExtent.width */
        .bufferImageHeight = 0,
        .imageSubresource  = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel       = 0,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
        .imageOffset = { (int32_t)x, (int32_t)y, 0 },
        .imageExtent = { w, h, 1 },
    };
    vkCmdCopyBufferToImage(cmd, ctx->staging_buffer, lt->image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    /* Transition back to SHADER_READ_ONLY */
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
}

/******************************************************************************************************************************
 * Initialization
 ******************************************************************************************************************************/

[[nodiscard]] static bool renderer_init(RendererContext* restrict ctx, GLFWwindow* restrict window, uint32_t width, uint32_t height)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->window = window;
    ctx->width  = width;
    ctx->height = height;

    /* --- Instance --- */
    uint32_t glfw_ext_count = 0;
    const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
    if (!glfw_exts) {
        fprintf(stderr, "[Vulkan] GLFW did not report required extensions\n");
        return false;
    }

    VkApplicationInfo app_info = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "FIELD",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 5),
        .pEngineName        = "Loom",
        .engineVersion      = VK_MAKE_VERSION(0, 1, 5),
        .apiVersion         = VK_API_VERSION_1_2,
    };

    VkInstanceCreateInfo inst_info = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &app_info,
        .enabledExtensionCount   = glfw_ext_count,
        .ppEnabledExtensionNames = glfw_exts,
    };

    if (vkCreateInstance(&inst_info, nullptr, &ctx->instance) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to create instance\n");
        return false;
    }

    /* --- Surface --- */
    if (glfwCreateWindowSurface(ctx->instance, window, nullptr, &ctx->surface) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to create window surface\n");
        return false;
    }

    /* --- Physical device --- */
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(ctx->instance, &device_count, nullptr);
    if (device_count == 0) {
        fprintf(stderr, "[Vulkan] No Vulkan-capable GPUs found\n");
        return false;
    }
    VkPhysicalDevice* devices = malloc(device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(ctx->instance, &device_count, devices);

    /* Pick the first device that supports graphics+present */
    ctx->physical_device = VK_NULL_HANDLE;
    for (uint32_t i = 0; i < device_count; ++i) {
        uint32_t gfx, present;
        if (vk_find_queue_families(devices[i], ctx->surface, &gfx, &present)) {
            /* Check swapchain support */
            uint32_t ext_count = 0;
            vkEnumerateDeviceExtensionProperties(devices[i], nullptr, &ext_count, nullptr);
            VkExtensionProperties* exts = malloc(ext_count * sizeof(VkExtensionProperties));
            vkEnumerateDeviceExtensionProperties(devices[i], nullptr, &ext_count, exts);
            bool has_swapchain = false;
            for (uint32_t e = 0; e < ext_count; ++e) {
                if (strcmp(exts[e].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                    has_swapchain = true;
                    break;
                }
            }
            free(exts);
            if (has_swapchain) {
                ctx->physical_device = devices[i];
                ctx->graphics_family = gfx;
                ctx->present_family  = present;
                break;
            }
        }
    }
    free(devices);

    if (ctx->physical_device == VK_NULL_HANDLE) {
        fprintf(stderr, "[Vulkan] No suitable GPU found\n");
        return false;
    }

    VkPhysicalDeviceProperties dev_props;
    vkGetPhysicalDeviceProperties(ctx->physical_device, &dev_props);
    printf("[Vulkan] Using device: %s\n", dev_props.deviceName);

    /* --- Logical device --- */
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_infos[2];
    uint32_t unique_families[2] = { ctx->graphics_family, ctx->present_family };
    uint32_t queue_count = (ctx->graphics_family == ctx->present_family) ? 1 : 2;

    for (uint32_t i = 0; i < queue_count; ++i) {
        queue_infos[i] = (VkDeviceQueueCreateInfo){
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = unique_families[i],
            .queueCount       = 1,
            .pQueuePriorities = &queue_priority,
        };
    }

    const char* device_exts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo dev_info = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount    = queue_count,
        .pQueueCreateInfos       = queue_infos,
        .enabledExtensionCount   = 1,
        .ppEnabledExtensionNames = device_exts,
    };

    if (vkCreateDevice(ctx->physical_device, &dev_info, nullptr, &ctx->device) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to create logical device\n");
        return false;
    }

    vkGetDeviceQueue(ctx->device, ctx->graphics_family, 0, &ctx->graphics_queue);
    vkGetDeviceQueue(ctx->device, ctx->present_family, 0, &ctx->present_queue);

    /* --- Swapchain --- */
    VkSurfaceFormatKHR surf_format;
    VkPresentModeKHR present_mode;
    VkExtent2D extent;
    if (!vk_query_swapchain_support(ctx->physical_device, ctx->surface,
                                     &surf_format, &present_mode, &extent)) {
        fprintf(stderr, "[Vulkan] Swapchain not supported\n");
        return false;
    }
    ctx->swapchain_format = surf_format.format;
    ctx->swapchain_extent = extent;

    uint32_t image_count = 3; /* triple buffering if available */
    {
        VkSurfaceCapabilitiesKHR caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physical_device, ctx->surface, &caps);
        if (caps.maxImageCount > 0 && image_count > caps.maxImageCount)
            image_count = caps.maxImageCount;
        if (image_count < caps.minImageCount)
            image_count = caps.minImageCount;
    }

    VkSwapchainCreateInfoKHR swap_info = {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = ctx->surface,
        .minImageCount    = image_count,
        .imageFormat      = surf_format.format,
        .imageColorSpace  = surf_format.colorSpace,
        .imageExtent      = extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = (ctx->graphics_family == ctx->present_family)
                              ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = (ctx->graphics_family == ctx->present_family) ? 0 : 2,
        .pQueueFamilyIndices   = (ctx->graphics_family == ctx->present_family) ? nullptr : unique_families,
        .preTransform   = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode    = present_mode,
        .clipped        = VK_TRUE,
        .oldSwapchain   = VK_NULL_HANDLE,
    };

    if (vkCreateSwapchainKHR(ctx->device, &swap_info, nullptr, &ctx->swapchain) != VK_SUCCESS) {
        fprintf(stderr, "[Vulkan] Failed to create swapchain\n");
        return false;
    }

    vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_image_count, nullptr);
    ctx->swapchain_images = malloc(ctx->swapchain_image_count * sizeof(VkImage));
    vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchain_image_count, ctx->swapchain_images);

    /* Create swapchain image views */
    ctx->swapchain_views = malloc(ctx->swapchain_image_count * sizeof(VkImageView));
    for (uint32_t i = 0; i < ctx->swapchain_image_count; ++i) {
        VkImageViewCreateInfo view_info = {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = ctx->swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = ctx->swapchain_format,
            .subresourceRange = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        vkCreateImageView(ctx->device, &view_info, nullptr, &ctx->swapchain_views[i]);
    }

    /* --- MSAA ---
     * Pick the highest supported color sample count (4x preferred) so line
     * primitives are antialiased; the render pass resolves the multisampled
     * color attachment into the swapchain image. Falls back to 1x (no
     * resolve attachment) if the device supports no color MSAA. */
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(ctx->physical_device, &props);
        VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts;
        ctx->msaa_samples = VK_SAMPLE_COUNT_1_BIT;
        if (counts & VK_SAMPLE_COUNT_4_BIT)      ctx->msaa_samples = VK_SAMPLE_COUNT_4_BIT;
        else if (counts & VK_SAMPLE_COUNT_2_BIT) ctx->msaa_samples = VK_SAMPLE_COUNT_2_BIT;
    }

    ctx->msaa_images   = nullptr;
    ctx->msaa_memories = nullptr;
    ctx->msaa_views    = nullptr;
    if (ctx->msaa_samples != VK_SAMPLE_COUNT_1_BIT) {
        ctx->msaa_images   = malloc(ctx->swapchain_image_count * sizeof(VkImage));
        ctx->msaa_memories = malloc(ctx->swapchain_image_count * sizeof(VkDeviceMemory));
        ctx->msaa_views    = malloc(ctx->swapchain_image_count * sizeof(VkImageView));
        for (uint32_t i = 0; i < ctx->swapchain_image_count; ++i) {
            VkImageCreateInfo img_info = {
                .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType     = VK_IMAGE_TYPE_2D,
                .format        = ctx->swapchain_format,
                .extent        = { extent.width, extent.height, 1 },
                .mipLevels     = 1,
                .arrayLayers   = 1,
                .samples       = ctx->msaa_samples,
                .tiling        = VK_IMAGE_TILING_OPTIMAL,
                .usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };
            if (vkCreateImage(ctx->device, &img_info, nullptr, &ctx->msaa_images[i]) != VK_SUCCESS) {
                fprintf(stderr, "[Vulkan] Failed to create MSAA color image\n");
                return false;
            }

            VkMemoryRequirements mem_reqs;
            vkGetImageMemoryRequirements(ctx->device, ctx->msaa_images[i], &mem_reqs);
            VkMemoryAllocateInfo alloc_info = {
                .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize  = mem_reqs.size,
                .memoryTypeIndex = vk_find_memory_type(ctx->physical_device,
                                                        mem_reqs.memoryTypeBits,
                                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
            };
            if (alloc_info.memoryTypeIndex == UINT32_MAX) return false;
            if (vkAllocateMemory(ctx->device, &alloc_info, nullptr, &ctx->msaa_memories[i]) != VK_SUCCESS) {
                fprintf(stderr, "[Vulkan] Failed to allocate MSAA image memory\n");
                return false;
            }
            vkBindImageMemory(ctx->device, ctx->msaa_images[i], ctx->msaa_memories[i], 0);

            VkImageViewCreateInfo view_info = {
                .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image    = ctx->msaa_images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format   = ctx->swapchain_format,
                .subresourceRange = {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
            };
            if (vkCreateImageView(ctx->device, &view_info, nullptr, &ctx->msaa_views[i]) != VK_SUCCESS) {
                fprintf(stderr, "[Vulkan] Failed to create MSAA image view\n");
                return false;
            }
        }
    }

    /* --- Render pass --- */
    /* When MSAA is active, attachment 0 is the multisampled color image and
       attachment 1 resolves it into the swapchain image, which becomes the
       presentable attachment. */
    const bool msaa = ctx->msaa_samples != VK_SAMPLE_COUNT_1_BIT;

    VkAttachmentDescription attachments[2] = {
        {
            .format         = ctx->swapchain_format,
            .samples        = ctx->msaa_samples,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = msaa ? VK_ATTACHMENT_STORE_OP_DONT_CARE
                                   : VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = msaa ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                                   : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
        {
            .format         = ctx->swapchain_format,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
    };

    VkAttachmentReference color_ref = {
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference resolve_ref = {
        .attachment = 1,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = 1,
        .pColorAttachments       = &color_ref,
        .pResolveAttachments     = msaa ? &resolve_ref : nullptr,
    };

    VkSubpassDependency dependency = {
        .srcSubpass    = VK_SUBPASS_EXTERNAL,
        .dstSubpass    = 0,
        .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo rp_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = msaa ? 2u : 1u,
        .pAttachments    = attachments,
        .subpassCount    = 1,
        .pSubpasses      = &subpass,
        .dependencyCount = 1,
        .pDependencies   = &dependency,
    };
    vkCreateRenderPass(ctx->device, &rp_info, nullptr, &ctx->render_pass);

    /* --- Framebuffers --- */
    ctx->framebuffers = malloc(ctx->swapchain_image_count * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < ctx->swapchain_image_count; ++i) {
        VkImageView fb_attachments[2] = {
            msaa ? ctx->msaa_views[i] : ctx->swapchain_views[i],
            ctx->swapchain_views[i],
        };
        VkFramebufferCreateInfo fb_info = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = ctx->render_pass,
            .attachmentCount = msaa ? 2u : 1u,
            .pAttachments    = fb_attachments,
            .width           = extent.width,
            .height          = extent.height,
            .layers          = 1,
        };
        vkCreateFramebuffer(ctx->device, &fb_info, nullptr, &ctx->framebuffers[i]);
    }

    /* --- Descriptor set layout (layer: combined image sampler) --- */
    VkDescriptorSetLayoutBinding layer_binding = {
        .binding         = 0,
        .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT,
    };
    VkDescriptorSetLayoutCreateInfo dsl_info = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings    = &layer_binding,
    };
    vkCreateDescriptorSetLayout(ctx->device, &dsl_info, nullptr, &ctx->layer_descriptor_set_layout);

    /* --- Descriptor pool (one set per layer texture) --- */
    VkDescriptorPoolSize pool_size = {
        .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = RENDERER_LAYER_COUNT,
    };
    VkDescriptorPoolCreateInfo dp_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets       = RENDERER_LAYER_COUNT,
        .poolSizeCount = 1,
        .pPoolSizes    = &pool_size,
    };
    vkCreateDescriptorPool(ctx->device, &dp_info, nullptr, &ctx->descriptor_pool);

    /* Allocate descriptor sets for each layer */
    VkDescriptorSetLayout layouts[RENDERER_LAYER_COUNT];
    for (int i = 0; i < RENDERER_LAYER_COUNT; ++i) layouts[i] = ctx->layer_descriptor_set_layout;
    VkDescriptorSetAllocateInfo ds_alloc = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = ctx->descriptor_pool,
        .descriptorSetCount = RENDERER_LAYER_COUNT,
        .pSetLayouts        = layouts,
    };
    VkDescriptorSet sets[RENDERER_LAYER_COUNT];
    vkAllocateDescriptorSets(ctx->device, &ds_alloc, sets);
    for (int i = 0; i < RENDERER_LAYER_COUNT; ++i)
        ctx->layers[i].descriptor_set = sets[i];

    /* --- Layer pipeline (textured quad) --- */
    {
        VkShaderModule vs = vk_create_shader_module(ctx->device, layer_vert_spv, layer_vert_spv_len);
        VkShaderModule fs = vk_create_shader_module(ctx->device, layer_frag_spv, layer_frag_spv_len);

        VkPipelineShaderStageCreateInfo stages[2] = {
            { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
              .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vs, .pName = "main" },
            { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
              .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = fs, .pName = "main" },
        };

        VkVertexInputBindingDescription bind_desc = {
            .binding   = 0,
            .stride    = 4 * sizeof(float), /* pos.xy + uv.xy */
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
        VkVertexInputAttributeDescription attr_descs[2] = {
            { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0 },
            { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 2 * sizeof(float) },
        };
        VkPipelineVertexInputStateCreateInfo vi = {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 1,
            .pVertexBindingDescriptions      = &bind_desc,
            .vertexAttributeDescriptionCount = 2,
            .pVertexAttributeDescriptions    = attr_descs,
        };

        VkPipelineInputAssemblyStateCreateInfo ia = {
            .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        };

        VkPipelineViewportStateCreateInfo vp = {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount  = 1,
        };

        VkPipelineRasterizationStateCreateInfo rs = {
            .sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode    = VK_CULL_MODE_NONE,
            .frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .lineWidth   = 1.0f,
        };

        VkPipelineMultisampleStateCreateInfo ms = {
            .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = ctx->msaa_samples,
        };

        VkPipelineColorBlendAttachmentState blend_att = {
            .blendEnable         = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp        = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .alphaBlendOp        = VK_BLEND_OP_ADD,
            .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                   VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };
        VkPipelineColorBlendStateCreateInfo cb = {
            .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments    = &blend_att,
        };

        VkPipelineLayoutCreateInfo pl_info = {
            .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts    = &ctx->layer_descriptor_set_layout,
        };
        vkCreatePipelineLayout(ctx->device, &pl_info, nullptr, &ctx->layer_pipeline_layout);

        VkDynamicState dyn_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dyn = {
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates    = dyn_states,
        };

        VkGraphicsPipelineCreateInfo pipe_info = {
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount          = 2,
            .pStages             = stages,
            .pVertexInputState   = &vi,
            .pInputAssemblyState = &ia,
            .pViewportState      = &vp,
            .pRasterizationState = &rs,
            .pMultisampleState   = &ms,
            .pColorBlendState    = &cb,
            .pDynamicState       = &dyn,
            .layout              = ctx->layer_pipeline_layout,
            .renderPass          = ctx->render_pass,
            .subpass             = 0,
        };
        VkResult res = vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1,
                                                  &pipe_info, nullptr, &ctx->layer_pipeline);
        vkDestroyShaderModule(ctx->device, vs, nullptr);
        vkDestroyShaderModule(ctx->device, fs, nullptr);
        if (res != VK_SUCCESS) {
            fprintf(stderr, "[Vulkan] Failed to create layer pipeline\n");
            return false;
        }
    }

    /* --- Line pipeline (colored line strip, push constant color) --- */
    {
        VkShaderModule vs = vk_create_shader_module(ctx->device, line_vert_spv, line_vert_spv_len);
        VkShaderModule fs = vk_create_shader_module(ctx->device, line_frag_spv, line_frag_spv_len);

        VkPipelineShaderStageCreateInfo stages[2] = {
            { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
              .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vs, .pName = "main" },
            { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
              .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = fs, .pName = "main" },
        };

        VkPushConstantRange push_range = {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset     = 0,
            .size       = sizeof(float) * 8, /* vec4 uColor + vec4 uFade */
        };
        VkPipelineLayoutCreateInfo pl_info = {
            .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges    = &push_range,
        };
        vkCreatePipelineLayout(ctx->device, &pl_info, nullptr, &ctx->line_pipeline_layout);

        VkVertexInputBindingDescription bind_desc = {
            .binding   = 0,
            .stride    = 2 * sizeof(float),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
        VkVertexInputAttributeDescription attr_desc = {
            .location = 0,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32_SFLOAT,
            .offset   = 0,
        };
        VkPipelineVertexInputStateCreateInfo vi = {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 1,
            .pVertexBindingDescriptions      = &bind_desc,
            .vertexAttributeDescriptionCount = 1,
            .pVertexAttributeDescriptions    = &attr_desc,
        };

        VkPipelineInputAssemblyStateCreateInfo ia = {
            .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
        };

        VkPipelineViewportStateCreateInfo vp = {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount  = 1,
        };

        VkPipelineRasterizationStateCreateInfo rs = {
            .sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode    = VK_CULL_MODE_NONE,
            .frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .lineWidth   = 1.0f,
        };

        VkPipelineMultisampleStateCreateInfo ms = {
            .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = ctx->msaa_samples,
        };

        VkPipelineColorBlendAttachmentState blend_att = {
            .blendEnable         = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp        = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .alphaBlendOp        = VK_BLEND_OP_ADD,
            .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                   VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };
        VkPipelineColorBlendStateCreateInfo cb = {
            .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments    = &blend_att,
        };

        VkDynamicState dyn_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dyn = {
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates    = dyn_states,
        };

        VkGraphicsPipelineCreateInfo pipe_info = {
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount          = 2,
            .pStages             = stages,
            .pVertexInputState   = &vi,
            .pInputAssemblyState = &ia,
            .pViewportState      = &vp,
            .pRasterizationState = &rs,
            .pMultisampleState   = &ms,
            .pColorBlendState    = &cb,
            .pDynamicState       = &dyn,
            .layout              = ctx->line_pipeline_layout,
            .renderPass          = ctx->render_pass,
            .subpass             = 0,
        };
        VkResult res = vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1,
                                                  &pipe_info, nullptr, &ctx->line_pipeline);
        vkDestroyShaderModule(ctx->device, vs, nullptr);
        vkDestroyShaderModule(ctx->device, fs, nullptr);
        if (res != VK_SUCCESS) {
            fprintf(stderr, "[Vulkan] Failed to create line pipeline\n");
            return false;
        }
    }

    /* --- Full-screen quad VBO --- */
    {
        static const float quad_data[] = {
            /* pos.x   pos.y    uv.x  uv.y */
            -1.0f, -1.0f,  0.0f, 1.0f,
            -1.0f,  1.0f,  0.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 0.0f,
            -1.0f, -1.0f,  0.0f, 1.0f,
             1.0f,  1.0f,  1.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 1.0f,
        };
        VkDeviceSize size = sizeof(quad_data);
        vk_create_buffer(ctx->device, ctx->physical_device, size,
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &ctx->quad_vbo, &ctx->quad_vbo_memory);
        void* mapped;
        vkMapMemory(ctx->device, ctx->quad_vbo_memory, 0, size, 0, &mapped);
        memcpy(mapped, quad_data, size);
        vkUnmapMemory(ctx->device, ctx->quad_vbo_memory);
    }

    /* --- Line VBO (dynamic, pre-allocated for all draws in a frame) --- */
    {
        vk_create_buffer(ctx->device, ctx->physical_device, VK_LINE_VBO_SIZE,
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &ctx->line_vbo, &ctx->line_vbo_memory);
    }

    /* --- Staging buffer (large enough for all layers at once) ---
     * Per-frame updates record copies for multiple layers into a single
     * command buffer. Each layer gets its own slot so the CPU can write
     * all pixel data before the GPU executes any copy. */
    {
        VkDeviceSize layer_size = (VkDeviceSize)width * height * sizeof(uint32_t);
        VkDeviceSize size = layer_size * RENDERER_LAYER_COUNT;
        vk_create_buffer(ctx->device, ctx->physical_device, size,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &ctx->staging_buffer, &ctx->staging_memory);
        ctx->staging_size = size;
        vkMapMemory(ctx->device, ctx->staging_memory, 0, size, 0, &ctx->staging_mapped);
    }

    /* --- Command pool --- */
    VkCommandPoolCreateInfo cp_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = ctx->graphics_family,
    };
    vkCreateCommandPool(ctx->device, &cp_info, nullptr, &ctx->command_pool);

    /* --- Per-frame command buffers --- */
    VkCommandBufferAllocateInfo cb_alloc = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = ctx->command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = VK_MAX_FRAMES_IN_FLIGHT,
    };
    vkAllocateCommandBuffers(ctx->device, &cb_alloc, ctx->command_buffers);

    /* --- Synchronization objects --- */
    VkSemaphoreCreateInfo sem_info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    for (uint32_t i = 0; i < VK_MAX_FRAMES_IN_FLIGHT; ++i) {
        vkCreateSemaphore(ctx->device, &sem_info, nullptr, &ctx->image_available[i]);
        vkCreateSemaphore(ctx->device, &sem_info, nullptr, &ctx->render_finished[i]);
        vkCreateFence(ctx->device, &fence_info, nullptr, &ctx->in_flight_fences[i]);
    }

    /* --- Create layer textures and upload initial data --- */
    /* Layer indices: 0=BG, 1=NG, 2=FG, 3=TL, 4=ST, 5=DO */
    for (int i = 0; i < RENDERER_LAYER_COUNT; ++i) {
        vk_create_layer_texture(ctx, &ctx->layers[i], width, height);
    }

    return true;
}

/******************************************************************************************************************************
 * Per-frame rendering
 ******************************************************************************************************************************/

/* Push constant data for line shader (vec4 color + vec4 fade ramp). */
typedef struct {
    float r, g, b, a;       /* color; a = uniform alpha for solid lines */
    float fade_alpha0;      /* opacity at the first vertex */
    float fade_step;        /* opacity increment per vertex */
    float pad[2];
} VkLinePushConstants;

/* Begin a frame: acquire next swapchain image, reset command buffer.
 * Does NOT begin the render pass — texture updates must happen first
 * (vkCmdCopyBufferToImage is illegal inside a render pass). */
[[nodiscard]] static bool renderer_begin_frame(RendererContext* restrict ctx, uint32_t* restrict image_index)
{
    vkWaitForFences(ctx->device, 1, &ctx->in_flight_fences[ctx->current_frame],
                    VK_TRUE, UINT64_MAX);
    vkResetFences(ctx->device, 1, &ctx->in_flight_fences[ctx->current_frame]);

    VkResult res = vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX,
                                          ctx->image_available[ctx->current_frame],
                                          VK_NULL_HANDLE, image_index);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        return false; /* swapchain needs recreation */
    }
    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "[Vulkan] Failed to acquire swapchain image\n");
        return false;
    }

    vkResetCommandBuffer(ctx->command_buffers[ctx->current_frame], 0);
    ctx->line_vbo_offset = 0;

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    vkBeginCommandBuffer(ctx->command_buffers[ctx->current_frame], &begin_info);

    return true;
}

/* Begin the render pass. Call AFTER all texture updates are recorded. */
static void renderer_begin_render(RendererContext* restrict ctx, uint32_t image_index)
{
    VkClearValue clear = { .color = { {0.0f, 0.0f, 0.0f, 0.0f} } };
    VkRenderPassBeginInfo rp_begin = {
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass  = ctx->render_pass,
        .framebuffer = ctx->framebuffers[image_index],
        .renderArea  = { .offset = {0, 0}, .extent = ctx->swapchain_extent },
        .clearValueCount = 1,
        .pClearValues    = &clear,
    };
    vkCmdBeginRenderPass(ctx->command_buffers[ctx->current_frame], &rp_begin,
                         VK_SUBPASS_CONTENTS_INLINE);

    /* Set viewport — negative height flips Y so that normalized 0-1 Y-up
     * coords in the line shader map to screen Y-down correctly. */
    VkViewport viewport = {
        .x        = 0.0f,
        .y        = (float)ctx->swapchain_extent.height,
        .width    = (float)ctx->swapchain_extent.width,
        .height   = -(float)ctx->swapchain_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(ctx->command_buffers[ctx->current_frame], 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = ctx->swapchain_extent,
    };
    vkCmdSetScissor(ctx->command_buffers[ctx->current_frame], 0, 1, &scissor);
}

/* Draw a layer texture as a full-screen quad. Records into the current cmd buffer. */
static void renderer_draw_layer(RendererContext* restrict ctx, int layer_index)
{
    VkLayerTexture* lt = &ctx->layers[layer_index];
    if (!lt->valid) return;

    VkCommandBuffer cmd = ctx->command_buffers[ctx->current_frame];

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->layer_pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            ctx->layer_pipeline_layout, 0, 1,
                            &lt->descriptor_set, 0, nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &ctx->quad_vbo, &offset);
    vkCmdDraw(cmd, 6, 1, 0, 0);
}

/* Draw a line strip whose vertex opacity ramps linearly from `alpha0` at the
 * first vertex to `alpha1` at the last vertex. Records into the current cmd
 * buffer.
 * `data` is count*2 floats in 0-1 normalized coords (Y-up).
 * `scissor_l/t/w/h` optionally clips the draw (screen coords, Y-down from top).
 *
 * Each draw gets its own slot in the line VBO (via line_vbo_offset) so multiple
 * line draws in one frame don't overwrite each other's vertex data before the
 * GPU reads it. The offset is reset to 0 at the start of each frame. */
static void renderer_draw_line_fade(RendererContext* restrict ctx,
                                    const float* restrict data, uint32_t count,
                                    uint32_t color, float alpha0, float alpha1,
                                    int32_t scissor_l, int32_t scissor_t,
                                    uint32_t scissor_w, uint32_t scissor_h)
{
    if (count < 2) return;

    VkCommandBuffer cmd = ctx->command_buffers[ctx->current_frame];
    VkDeviceSize draw_size = (VkDeviceSize)count * 2 * sizeof(float);

    /* Bounds check: don't overflow the line VBO */
    if (ctx->line_vbo_offset + draw_size > VK_LINE_VBO_SIZE) return;

    /* Upload vertex data to this draw's slot in the line VBO */
    void* mapped;
    vkMapMemory(ctx->device, ctx->line_vbo_memory, ctx->line_vbo_offset,
                draw_size, 0, &mapped);
    memcpy(mapped, data, draw_size);
    vkUnmapMemory(ctx->device, ctx->line_vbo_memory);

    /* Set scissor if requested */
    if (scissor_w > 0 && scissor_h > 0) {
        VkRect2D scissor = {
            .offset = { scissor_l, scissor_t },
            .extent = { scissor_w, scissor_h },
        };
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    } else {
        VkRect2D scissor = {
            .offset = {0, 0},
            .extent = ctx->swapchain_extent,
        };
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->line_pipeline);

    /* Push constants: color as vec4 (0xRRGGBBAA → r,g,b,a in 0-1) plus the fade
     * ramp. The vertex shader computes alpha = fade_alpha0 + gl_VertexIndex *
     * fade_step, so a single draw can span the whole opacity ramp. */
    VkLinePushConstants pc = {
        .r = (float)ff_extract_byte(color, 3) / 255.0f,
        .g = (float)ff_extract_byte(color, 2) / 255.0f,
        .b = (float)ff_extract_byte(color, 1) / 255.0f,
        .a = (float)ff_extract_byte(color, 0) / 255.0f,
        .fade_alpha0 = alpha0,
        .fade_step   = (alpha1 - alpha0) / (float)(count - 1),
    };
    vkCmdPushConstants(cmd, ctx->line_pipeline_layout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

    vkCmdBindVertexBuffers(cmd, 0, 1, &ctx->line_vbo, &ctx->line_vbo_offset);
    vkCmdDraw(cmd, count, 1, 0, 0);

    /* Advance to the next slot (stride is 8 bytes, so always aligned) */
    ctx->line_vbo_offset += draw_size;
}

/* Draw a line strip with a uniform (solid) color. Thin wrapper over
 * renderer_draw_line_fade with a flat alpha ramp. */
static void renderer_draw_line(RendererContext* restrict ctx,
                               const float* restrict data, uint32_t count,
                               uint32_t color,
                               int32_t scissor_l, int32_t scissor_t,
                               uint32_t scissor_w, uint32_t scissor_h)
{
    const float a = (float)ff_extract_byte(color, 0) / 255.0f;
    renderer_draw_line_fade(ctx, data, count, color, a, a,
                            scissor_l, scissor_t, scissor_w, scissor_h);
}

/* Update a sub-region of a layer texture with new pixel data (dirty-rect
 * upload). Records into the current cmd buffer.
 * Must be called OUTSIDE a render pass (uses vkCmdCopyBufferToImage).
 * Each layer gets its own slot in the staging buffer per frame in flight, so
 * multiple updates in one command buffer don't clobber each other and a
 * frame's memcpy can't race the previous frame's in-flight copy. */
static void renderer_update_layer_rect(RendererContext* restrict ctx, int layer_index,
                                        const uint32_t* restrict pixels, uint32_t width, uint32_t height,
                                        uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    VkDeviceSize layer_size = (VkDeviceSize)width * height * sizeof(uint32_t);
    VkDeviceSize buffer_offset =
        ((VkDeviceSize)ctx->current_frame * RENDERER_LAYER_COUNT + (VkDeviceSize)layer_index) * layer_size;
    vk_update_layer_texture(ctx->command_buffers[ctx->current_frame], ctx,
                            &ctx->layers[layer_index], pixels, width,
                            x, y, w, h, buffer_offset);
}

/* End the frame: finish render pass, submit, present. */
static void renderer_end_frame(RendererContext* restrict ctx, uint32_t image_index)
{
    VkCommandBuffer cmd = ctx->command_buffers[ctx->current_frame];

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &ctx->image_available[ctx->current_frame],
        .pWaitDstStageMask    = &wait_stage,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &ctx->render_finished[ctx->current_frame],
    };
    vkQueueSubmit(ctx->graphics_queue, 1, &submit, ctx->in_flight_fences[ctx->current_frame]);

    VkPresentInfoKHR present = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &ctx->render_finished[ctx->current_frame],
        .swapchainCount     = 1,
        .pSwapchains        = &ctx->swapchain,
        .pImageIndices      = &image_index,
    };
    vkQueuePresentKHR(ctx->present_queue, &present);

    ctx->current_frame = (ctx->current_frame + 1) % VK_MAX_FRAMES_IN_FLIGHT;
}

/******************************************************************************************************************************
 * Cleanup
 ******************************************************************************************************************************/

static void renderer_destroy(RendererContext* restrict ctx)
{
    vkDeviceWaitIdle(ctx->device);

    /* Layer textures */
    for (int i = 0; i < RENDERER_LAYER_COUNT; ++i) {
        VkLayerTexture* lt = &ctx->layers[i];
        if (lt->sampler) vkDestroySampler(ctx->device, lt->sampler, nullptr);
        if (lt->view)    vkDestroyImageView(ctx->device, lt->view, nullptr);
        if (lt->image)   vkDestroyImage(ctx->device, lt->image, nullptr);
        if (lt->memory)  vkFreeMemory(ctx->device, lt->memory, nullptr);
    }

    /* Buffers */
    if (ctx->quad_vbo)        vkDestroyBuffer(ctx->device, ctx->quad_vbo, nullptr);
    if (ctx->quad_vbo_memory) vkFreeMemory(ctx->device, ctx->quad_vbo_memory, nullptr);
    if (ctx->line_vbo)        vkDestroyBuffer(ctx->device, ctx->line_vbo, nullptr);
    if (ctx->line_vbo_memory) vkFreeMemory(ctx->device, ctx->line_vbo_memory, nullptr);
    if (ctx->staging_buffer)  vkDestroyBuffer(ctx->device, ctx->staging_buffer, nullptr);
    if (ctx->staging_memory)  vkFreeMemory(ctx->device, ctx->staging_memory, nullptr);

    /* Sync objects */
    for (uint32_t i = 0; i < VK_MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(ctx->device, ctx->image_available[i], nullptr);
        vkDestroySemaphore(ctx->device, ctx->render_finished[i], nullptr);
        vkDestroyFence(ctx->device, ctx->in_flight_fences[i], nullptr);
    }

    /* Command pool */
    vkDestroyCommandPool(ctx->device, ctx->command_pool, nullptr);

    /* Pipelines */
    vkDestroyPipeline(ctx->device, ctx->layer_pipeline, nullptr);
    vkDestroyPipelineLayout(ctx->device, ctx->layer_pipeline_layout, nullptr);
    vkDestroyPipeline(ctx->device, ctx->line_pipeline, nullptr);
    vkDestroyPipelineLayout(ctx->device, ctx->line_pipeline_layout, nullptr);

    /* Descriptor pool (frees all sets) */
    vkDestroyDescriptorPool(ctx->device, ctx->descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(ctx->device, ctx->layer_descriptor_set_layout, nullptr);

    /* Framebuffers + image views (+ per-image MSAA color attachments) */
    for (uint32_t i = 0; i < ctx->swapchain_image_count; ++i) {
        vkDestroyFramebuffer(ctx->device, ctx->framebuffers[i], nullptr);
        vkDestroyImageView(ctx->device, ctx->swapchain_views[i], nullptr);
        if (ctx->msaa_views) {
            vkDestroyImageView(ctx->device, ctx->msaa_views[i], nullptr);
            vkDestroyImage(ctx->device, ctx->msaa_images[i], nullptr);
            vkFreeMemory(ctx->device, ctx->msaa_memories[i], nullptr);
        }
    }
    free(ctx->framebuffers);
    free(ctx->swapchain_views);
    free(ctx->swapchain_images);
    free(ctx->msaa_views);
    free(ctx->msaa_images);
    free(ctx->msaa_memories);

    /* Swapchain */
    vkDestroySwapchainKHR(ctx->device, ctx->swapchain, nullptr);

    /* Render pass */
    vkDestroyRenderPass(ctx->device, ctx->render_pass, nullptr);

    /* Device + surface + instance */
    vkDestroyDevice(ctx->device, nullptr);
    vkDestroySurfaceKHR(ctx->instance, ctx->surface, nullptr);
    vkDestroyInstance(ctx->instance, nullptr);
}


