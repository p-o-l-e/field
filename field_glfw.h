#pragma once

/******************************************************************************************************************************
 * Field GLFW frontend — window management, event handling, and the main
 * render loop. The actual GPU rendering is delegated to a backend selected
 * at build time:
 *
 *   LOOM_RENDERER=vulkan  (default) — field/vulkan_backend.h
 *   LOOM_RENDERER=opengl             — field/opengl_backend.h
 *
 * Both backends implement the same renderer interface (see the comment in
 * each backend header), so this file is backend-agnostic.
 ******************************************************************************************************************************/

/* Select and include the rendering backend.
 * The backend header pulls in its own dependencies (Vulkan or OpenGL
 * function loaders) and defines RendererContext + renderer_* functions. */
#if defined(LOOM_RENDERER_OPENGL)
    #include "opengl_backend.h"
    #define RENDERER_NAME "OpenGL 3.3 Core"
#elif defined(LOOM_RENDERER_VULKAN)
    #include "vulkan_backend.h"
    #define RENDERER_NAME "Vulkan"
#else
    #error "No renderer selected. Define LOOM_RENDERER_OPENGL or LOOM_RENDERER_VULKAN."
#endif

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define _MULTI_THREADED
#include <pthread.h>

#include "field.h"
#define FIELD_TRANSPARENCY 1

/* Renderer layer indices (compositing order, back to front).
 * These map to slots in the backend's layer texture array. */
enum {
    R_LAYER_BG = 0,
    R_LAYER_NG = 1,
    R_LAYER_FG = 2,
    R_LAYER_TL = 3,
    R_LAYER_ST = 4,
    R_LAYER_DO = 5,  /* only used when DEBUG_OVERLAY is defined */
};

/* Upload only the dirty region of a field layer to its GPU texture, then
 * reset the layer's dirty flag. Layers with no modifications since the last
 * upload are skipped entirely — this is what keeps per-frame bandwidth
 * proportional to what actually changed instead of the full layer size.
 * Note the renderer and field layer indices differ (R_LAYER_FG=2 vs FG=3),
 * so both indices are passed explicitly. */
static void ff_upload_dirty_layer(RendererContext* restrict ctx, const Field* restrict field, int r_layer, int f_layer) {
    Frame* frame = field->layer[f_layer];
    if (!frame->is_dirty) return;
    renderer_update_layer_rect(ctx, r_layer, frame->data, frame->width, frame->height,
                               frame->dirty.l, frame->dirty.t,
                               frame->dirty.r - frame->dirty.l + 1,
                               frame->dirty.b - frame->dirty.t + 1);
    frame->is_dirty = false;
}

/******************************************************************************************************************************
 * Threading globals
 ******************************************************************************************************************************/

GLFWwindow* window;

static bool _switch_on = true;
static bool _window_on = true;
static bool _handle_events = true;

pthread_t field_thread;
pthread_t event_thread;

pthread_mutex_t _main_lock;
pthread_mutex_t _screen_lock;

pthread_barrier_t _init_barrier;
pthread_barrier_t _exit_barrier;

pthread_cond_t _repaint_condition;
pthread_cond_t _escape_condition;

/* Pending window resize — recorded by framebuffer_size_callback on the event
 * thread and applied by the draw thread (the only thread that touches the
 * renderer, so no GPU object is recreated mid-frame). A zero width means
 * "no resize pending". */
static atomic_uint _pending_width  = 0;
static atomic_uint _pending_height = 0;

/******************************************************************************************************************************
 * GLFW callbacks (shared across backends)
 ******************************************************************************************************************************/

static RendererContext g_renderer;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    (void)window;
    /* Minimised windows report 0x0 — keep the last valid size. */
    if (width <= 0 || height <= 0) return;

    printf("[resize-cb] framebuffer %dx%d\n", width, height); /* DIAGNOSTIC */
    pthread_mutex_lock(&_screen_lock);
    atomic_store(&_pending_width, (uint32_t)width);
    atomic_store(&_pending_height, (uint32_t)height);
    pthread_cond_signal(&_repaint_condition);
    pthread_mutex_unlock(&_screen_lock);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Field* context = (Field*)glfwGetWindowUserPointer(window);

    /* Input callbacks run on the event thread and read/mutate field state
     * (stencil hit tests, entity values). They take _screen_lock so they
     * serialize with the draw thread's ff_resize_field, which frees and
     * reallocates the layer buffers those hit tests read — without this, a
     * scroll event racing a resize reads a freed stencil buffer. */
    pthread_mutex_lock(&_screen_lock);
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    ff_hit_test(context, (int)x, (int)y);
    if(context->current[IP_ENTITY] != 0)
    {
        auto cn = context->current[IP_NODE];
        auto ce = context->current[IP_ENTITY];
        scroll_entity[context->node[cn].at[ce].type](&context->node[cn].at[ce], (int)xoffset, (int)yoffset);
    }
    context->current[IP_ENTITY] = 0;
    pthread_mutex_unlock(&_screen_lock);
}

static void cursor_position_callback(GLFWwindow* window, double x, double y)
{
    Field* context = (Field*)glfwGetWindowUserPointer(window);

    /* See scroll_callback: serialize with ff_resize_field on the draw thread. */
    pthread_mutex_lock(&_screen_lock);
    if(context->drag)
        ff_hit_test_drag(context, (int)x, (int)y);
    else
        ff_hit_test(context, (int)x, (int)y);
    pthread_mutex_unlock(&_screen_lock);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int/* mods*/)
{
    Field* context = (Field*)glfwGetWindowUserPointer(window);

    /* See scroll_callback: serialize with ff_resize_field on the draw thread. */
    pthread_mutex_lock(&_screen_lock);
    switch(button)
    {
        case GLFW_MOUSE_BUTTON_LEFT:
        {
            if(action == GLFW_PRESS) {
                double x, y;
                glfwGetCursorPos(window, &x, &y);

                ff_hit_test_down(context, (int)x, (int)y, LMB);
                auto cn = context->current[IP_NODE];
                auto ce = context->current[IP_ENTITY];
                if(context->node[cn].at[ce].flags & MOVEABLE)
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

            }
            else if(action == GLFW_RELEASE) {
                double x, y;

                glfwGetCursorPos(window, &x, &y);
                ff_hit_test_up(context, (int)x, (int)y, LMB);
                context->move = false;

                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
        break;

        case GLFW_MOUSE_BUTTON_RIGHT:
        {
            if(action == GLFW_PRESS) {
                double x, y;
                glfwGetCursorPos(window, &x, &y);
                ff_hit_test_down(context, (int)x, (int)y, RMB);
            }
            else if(action == GLFW_RELEASE)
                printf("RMB Released\n");
        }
        break;

        case GLFW_MOUSE_BUTTON_MIDDLE:
        {
            if(action == GLFW_PRESS)
                printf("MMB Pressed\n");
            else if(action == GLFW_RELEASE)
                printf("MMB Released\n");
        }
        break;

    }
    pthread_mutex_unlock(&_screen_lock);
}

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
    printf("Error: %d\n", error);
}

static void key_callback(GLFWwindow* /*window*/, int key, int/* scancode*/, int action, int/* mods*/)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        _window_on = false;
    }
}

void window_pos_callback(GLFWwindow* window, int, int)
{
    (void)window;
}

/******************************************************************************************************************************
 * Thread functions
 ******************************************************************************************************************************/

void* events_process(void*)
{
    pthread_barrier_wait(&_init_barrier);

    while(_handle_events)
    {
        glfwWaitEvents();
        pthread_cond_signal(&_repaint_condition);
    }

    printf("Event handler terminated...\n");
    pthread_barrier_wait(&_exit_barrier);

    return nullptr;
}

void* draw_field(void* arg)
{
    glfwSetErrorCallback(error_callback);

    glfwInitHint(GLFW_WAYLAND_LIBDECOR, GLFW_WAYLAND_DISABLE_LIBDECOR);
    if (!glfwInit()) exit(EXIT_FAILURE);

    printf("Compiled against GLFW %i.%i.%i\n", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);
    printf("%s\n", glfwGetVersionString());

#if defined(LOOM_RENDERER_OPENGL)
    /* Request an OpenGL 3.3 Core Profile context */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    /* Vulkan: no API context needed */
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_POSITION_X, 100);
    glfwWindowHint(GLFW_POSITION_Y, 100);

    Field* context = (Field*)arg;

    window = glfwCreateWindow((int)context->layer[FG]->width, (int)context->layer[FG]->height, "FIELD", nullptr, nullptr);

    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    /* Minimum size only — the window is freely resizable; anchored nodes
     * follow the window borders (see ff_resize_field). */
    glfwSetWindowSizeLimits(window, 320, 240, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetWindowSize(window, context->layer[FG]->width, context->layer[FG]->height);

    glfwSetWindowUserPointer(window, context);
#if defined(LOOM_RENDERER_OPENGL)
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
#endif
    glfwSetWindowPosCallback(window, window_pos_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    /* Initialize the rendering backend */
    if (!renderer_init(&g_renderer, window, context->layer[FG]->width, context->layer[FG]->height))
    {
        fprintf(stderr, "Failed to initialize %s backend\n", RENDERER_NAME);
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    printf("Renderer: %s (%ux%u)\n", RENDERER_NAME,
           context->layer[FG]->width, context->layer[FG]->height);

    /* Upload initial layer pixel data to GPU textures */
    renderer_upload_layer(&g_renderer, R_LAYER_BG, context->layer[BG]->data, context->layer[BG]->width, context->layer[BG]->height);
    renderer_upload_layer(&g_renderer, R_LAYER_NG, context->layer[NG]->data, context->layer[NG]->width, context->layer[NG]->height);
    renderer_upload_layer(&g_renderer, R_LAYER_FG, context->layer[FG]->data, context->layer[FG]->width, context->layer[FG]->height);
    renderer_upload_layer(&g_renderer, R_LAYER_TL, context->layer[TL]->data, context->layer[TL]->width, context->layer[TL]->height);
    renderer_upload_layer(&g_renderer, R_LAYER_ST, context->layer[ST]->data, context->layer[ST]->width, context->layer[ST]->height);
    #ifdef DEBUG_OVERLAY
        renderer_upload_layer(&g_renderer, R_LAYER_DO, context->layer[DO]->data, context->layer[DO]->width, context->layer[DO]->height);
    #endif

    constexpr uint32_t CORD_NORMAL = GRAY;
    constexpr uint32_t CORD_SELECT = GREEN;

    pthread_barrier_wait(&_init_barrier);
    printf("GLFW Initialized...\n");

    struct timespec loop_start;
    clock_gettime(CLOCK_MONOTONIC, &loop_start);

    while (_window_on)
    {
        /* DIAGNOSTIC HOOK — not for production: LOOM_TEST_RESIZE1/2 hold
         * "WxH@SEC" (seconds after this loop starts). When the loop has been
         * running SEC seconds the window is resized via glfwSetWindowSize,
         * called on GLFW's main thread (this draw thread). The resulting
         * ConfigureNotify drives the same framebuffer_size_callback ->
         * ff_resize_field path as a WM resize. Used to reproduce resize
         * behaviour without a window manager.
         * LOOM_TEST_BURST="WxH@SEC:COUNT" additionally fires one resize per
         * loop iteration for COUNT iterations, alternating WxH with the
         * startup size — mimics a continuous WM resize drag. */
        static bool hook_init = false;
        static bool done1 = false, done2 = false;
        static long at1 = -1, at2 = -1;
        static unsigned rw1, rh1, rw2, rh2;
        static long burst_at = -1;
        static int  burst_count = 0, burst_left = 0;
        static unsigned bw, bh;
        static bool burst_armed = false;
        if (!hook_init) {
            hook_init = true;
            const char* s1 = getenv("LOOM_TEST_RESIZE1");
            const char* s2 = getenv("LOOM_TEST_RESIZE2");
            const char* sb = getenv("LOOM_TEST_BURST");
            if (s1) sscanf(s1, "%ux%u@%ld", &rw1, &rh1, &at1);
            if (s2) sscanf(s2, "%ux%u@%ld", &rw2, &rh2, &at2);
            if (sb) sscanf(sb, "%ux%u@%ld:%d", &bw, &bh, &burst_at, &burst_count);
        }
        if (at1 >= 0 || at2 >= 0 || burst_at >= 0) {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            const long elapsed = now.tv_sec - loop_start.tv_sec;
            if (!done1 && at1 >= 0 && elapsed >= at1) {
                done1 = true;
                printf("[test-resize] glfwSetWindowSize %ux%u after %lds\n", rw1, rh1, elapsed);
                glfwSetWindowSize(window, (int)rw1, (int)rh1);
            }
            if (!done2 && at2 >= 0 && elapsed >= at2) {
                done2 = true;
                printf("[test-resize] glfwSetWindowSize %ux%u after %lds\n", rw2, rh2, elapsed);
                glfwSetWindowSize(window, (int)rw2, (int)rh2);
            }
            if (!burst_armed && burst_at >= 0 && elapsed >= burst_at) {
                burst_armed = true;
                burst_left = burst_count;
            }
            if (burst_left > 0) {
                if (burst_left & 1)
                    glfwSetWindowSize(window, (int)bw, (int)bh);
                else
                    glfwSetWindowSize(window, 1280, 720); /* main.c startup size */
                --burst_left;
            }
        }

        /* When the Oscillograph has valid streaming data, wake at its configured
         * frame rate instead of blocking indefinitely. */
        Oscillograph* osc = nullptr;
        if (context->stream[0]) {
            osc = (Oscillograph*)context->stream[0]->extension;
        }
        bool stream_active = osc && osc->x && *osc->x && osc->y && *osc->y;

        pthread_mutex_lock(&_screen_lock);
        if (stream_active) {
            uint32_t fps = osc->fps > 0 ? osc->fps : 60;
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec += 1000000000L / fps;
            if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; }
            pthread_cond_timedwait(&_repaint_condition, &_screen_lock, &ts);
        } else {
            pthread_cond_wait(&_repaint_condition, &_screen_lock);
        }
        pthread_mutex_unlock(&_screen_lock);

        /* Apply a pending window resize (recorded by the event thread) before
         * anything renders: resize the field and its anchored nodes, recreate
         * the renderer's extent-dependent objects, and re-upload all layers
         * (fresh textures need a full upload before the per-frame dirty-rect
         * path can update them). The pending flag is claimed under
         * _screen_lock — the same lock the callback holds while recording —
         * so a resize arriving mid-check is never lost. The resize itself
         * runs outside the lock. */
        uint32_t rw = 0, rh = 0;
        pthread_mutex_lock(&_screen_lock);
        if (atomic_load(&_pending_width)) {
            rw = atomic_load(&_pending_width);
            rh = atomic_load(&_pending_height);
            atomic_store(&_pending_width, 0);
            /* Apply the field resize under _screen_lock: ff_resize_field frees
             * and reallocates the layer buffers and rewrites entity bounds and
             * stencils, all of which the event thread's input callbacks read
             * (stencil hit tests). Without this serialization a cursor/scroll/
             * click event racing the resize reads a freed stencil buffer
             * (use-after-free). The GPU-side resize and re-upload stay outside
             * the lock — the event thread never touches renderer objects. */
            printf("[resize] applying %ux%u\n", rw, rh); /* DIAGNOSTIC */
            ff_resize_field(context, rw, rh);
        }
        pthread_mutex_unlock(&_screen_lock);

        if (rw) {
            const bool rok = renderer_resize(&g_renderer, window, rw, rh);
            printf("[resize] renderer_resize=%d\n", (int)rok); /* DIAGNOSTIC */
            if (rok) {
                renderer_upload_layer(&g_renderer, R_LAYER_BG, context->layer[BG]->data, rw, rh);
                renderer_upload_layer(&g_renderer, R_LAYER_NG, context->layer[NG]->data, rw, rh);
                renderer_upload_layer(&g_renderer, R_LAYER_FG, context->layer[FG]->data, rw, rh);
                renderer_upload_layer(&g_renderer, R_LAYER_TL, context->layer[TL]->data, rw, rh);
                renderer_upload_layer(&g_renderer, R_LAYER_ST, context->layer[ST]->data, rw, rh);
                #ifdef DEBUG_OVERLAY
                    renderer_upload_layer(&g_renderer, R_LAYER_DO, context->layer[DO]->data, rw, rh);
                #endif
            }
            context->repaint = true;
        }

        #ifdef DEBUG_OVERLAY
            if(atomic_load(&force_repaint)) context->repaint = true;
        #endif

        bool stream_updated = false;

        if(atomic_load_explicit(&trigger_stream_update, memory_order_acquire)) {
            atomic_store_explicit(&trigger_stream_update, false, memory_order_release);
            context->repaint = true;
            stream_updated = true;
        }

        bool scene_dirty = false;
        if(context->repaint)
        {
            stream_updated = true;
            ff_draw_scene(context);
            scene_dirty = true;
        }

        /* Feed the oscillograph's ring buffer with the latest XY sample */
        if (stream_updated && context->stream[0]) {
            auto ext = (Oscillograph*)context->stream[0]->extension;
            if (ext->x && *ext->x && ext->y && *ext->y) {
                float scale = (float)context->stream[0]->width / (float)context->width;
                float cx = (float)(context->stream[0]->bounds.l + context->stream[0]->width / 2) / (float)context->width;
                float cy = 1.0f - (float)(context->stream[0]->bounds.t + context->stream[0]->height / 2) / (float)context->height;

                auto buf = (float*)ext->data;
                buf[ext->w_head++] = **ext->x * scale + cx;
                buf[ext->w_head++] = **ext->y * scale + cy;
                if (ext->w_head >= ext->size * 2) ext->w_head = 0;
                if (ext->r_head < ext->size) ++ext->r_head;
            }
        }

        bool draw_connecting = context->connecting && context->pressed &&
            ((Socket*)context->pressed->extension)->cord;
        bool draw_cords = false;
        for(uint32_t n = 0; n < context->nodes && !draw_cords; ++n) {
            for(uint32_t i = 0; i < context->node[n].entities; ++i) {
                auto entity = &context->node[n].at[i];
                if(entity->type == ST_SOCKET) {
                    auto ext = (Socket*)entity->extension;
                    if(ext->connected && ext->has_data) { draw_cords = true; break; }
                }
            }
        }

        bool need_frame = scene_dirty || stream_updated || draw_connecting || draw_cords;
        if (!need_frame) continue;

        uint32_t image_index;
        if (!renderer_begin_frame(&g_renderer, &image_index))
            continue;

        /* Update layer textures BEFORE rendering. Only the dirty region of
         * each layer is uploaded (dirty-rect texturing); clean layers are
         * skipped entirely.
         * Vulkan: must be outside the render pass (transfer commands illegal inside).
         * OpenGL: just glTexSubImage2D, order doesn't matter but we keep it consistent. */
        if (scene_dirty) {
            ff_upload_dirty_layer(&g_renderer, context, R_LAYER_BG, BG);
            ff_upload_dirty_layer(&g_renderer, context, R_LAYER_NG, NG);
            ff_upload_dirty_layer(&g_renderer, context, R_LAYER_FG, FG);
            ff_upload_dirty_layer(&g_renderer, context, R_LAYER_TL, TL);

            if (context->staging) {
                ff_upload_dirty_layer(&g_renderer, context, R_LAYER_ST, ST);
            }

            #ifdef DEBUG_OVERLAY
                if(atomic_load_explicit(&force_repaint, memory_order_acquire)) {
                    atomic_store_explicit(&force_repaint, false, memory_order_release);
                }
                ff_upload_dirty_layer(&g_renderer, context, R_LAYER_DO, DO);
            #endif
        }

        /* Begin render pass (Vulkan) / no-op (OpenGL) */
        renderer_begin_render(&g_renderer, image_index);

        /* Composite layers back-to-front. Always redraw — Vulkan clears the
         * swapchain each frame; OpenGL was cleared in begin_frame. */
        renderer_draw_layer(&g_renderer, R_LAYER_BG);
        renderer_draw_layer(&g_renderer, R_LAYER_NG);
        renderer_draw_layer(&g_renderer, R_LAYER_FG);
        renderer_draw_layer(&g_renderer, R_LAYER_TL);

        if (context->staging) {
            renderer_draw_layer(&g_renderer, R_LAYER_ST);
        }

        #ifdef DEBUG_OVERLAY
            renderer_draw_layer(&g_renderer, R_LAYER_DO);
        #endif

        /* Draw oscillograph stream line (scissored to widget bounds), CRT-style:
         * the head (newest point) is fully opaque and opacity fades linearly to
         * zero at the tail (oldest point) — phosphor decay behind the beam. The
         * fade is ONE continuous ramp alpha(g) = g/(N-1) over the chronological
         * point index g (N = r_head valid points), so it stays seamless across
         * the ring wrap where the trace is drawn as two segments. Gating on the
         * valid-point count avoids blanking the trace for two frames after every
         * ring wrap (the write cursor resets to 0) — that was the periodic blink.
         * And once the ring has wrapped, chronological order starts at the write
         * cursor, not at index 0; the wrap seam between the two segments is
         * drawn explicitly so no sector of the trace is ever missing. Both
         * backends skip draws with count < 2. */
        if (context->stream[0] && ((Oscillograph*)context->stream[0]->extension)->r_head >= 2) {
            auto s = context->stream[0];
            auto ext = (Oscillograph*)s->extension;
            auto buf = (const float*)ext->data;
            const float inv = 1.0f / ((float)ext->r_head - 1.0f);
            if (ext->r_head < ext->size) {
                /* Still filling — points are contiguous from the start. */
                renderer_draw_line_fade(&g_renderer, buf, ext->r_head,
                                        0x40A090FF, 0.0f, 1.0f,
                                        (int32_t)s->bounds.l, (int32_t)s->bounds.t,
                                        s->width, s->height);
            } else {
                /* Ring full — oldest point sits at the write cursor. Segment A
                 * covers chronological indices [0, countA), segment B [countA, N),
                 * and countA + countB == size == r_head, so the two per-segment
                 * ramps join exactly into the global 0.0 → 1.0 fade. */
                const uint32_t count_a = (ext->size * 2 - ext->w_head) / 2;
                const uint32_t count_b = ext->w_head / 2;
                renderer_draw_line_fade(&g_renderer, buf + ext->w_head,
                                        count_a,
                                        0x40A090FF,
                                        0.0f,
                                        (float)(count_a - 1) * inv,
                                        (int32_t)s->bounds.l, (int32_t)s->bounds.t,
                                        s->width, s->height);
                renderer_draw_line_fade(&g_renderer, buf, count_b,
                                        0x40A090FF,
                                        (float)count_a * inv,
                                        1.0f,
                                        (int32_t)s->bounds.l, (int32_t)s->bounds.t,
                                        s->width, s->height);
                /* The ring wraps between segment A and segment B, so the sector
                 * connecting them (P[size-1] → P[0]) lies in no contiguous run —
                 * without it the trace has a one-sector gap that rotates with the
                 * write cursor. It also carries the head/tail point when the other
                 * segment is too short to draw on its own (count < 2). Skipped
                 * when w_head == 0: then segment A is the whole trace and this
                 * sector would wrongly close the loop newest → oldest. */
                if (count_b >= 1) {
                    const float seam[4] = {
                        buf[ext->size * 2 - 2], buf[ext->size * 2 - 1],
                        buf[0], buf[1],
                    };
                    renderer_draw_line_fade(&g_renderer, seam, 2,
                                            0x40A090FF,
                                            (float)(count_a - 1) * inv,
                                            (float)count_a * inv,
                                            (int32_t)s->bounds.l, (int32_t)s->bounds.t,
                                            s->width, s->height);
                }
            }
        }

        /* Draw pending connection cord */
        if (draw_connecting) {
            auto ext = (Socket*)context->pressed->extension;
            renderer_draw_line(&g_renderer,
                (const float*)ext->cord, SPLINE_SEGMENTS,
                CORD_SELECT, 0, 0, 0, 0);
        }

        /* Draw established connection cords */
        for(uint32_t n = 0; n < context->nodes; ++n) {
            for(uint32_t i = 0; i < context->node[n].entities; ++i) {
                auto entity = &context->node[n].at[i];
                if(entity->type == ST_SOCKET) {
                    auto ext = (Socket*)entity->extension;
                    if(ext->connected && ext->has_data) {
                        auto colour = entity->hovered || ext->connection->hovered ? CORD_SELECT : CORD_NORMAL;
                        renderer_draw_line(&g_renderer,
                            (const float*)ext->cord, SPLINE_SEGMENTS,
                            colour, 0, 0, 0, 0);
                    }
                }
            }
        }

        renderer_end_frame(&g_renderer, image_index);
    }

    /* Cleanup */
    renderer_destroy(&g_renderer);

    pthread_cond_signal(&_escape_condition);

    _switch_on      = false;
    _handle_events  = false;
    glfwPostEmptyEvent();

    ffDestroyField(context);
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    glfwDestroyWindow(window);

    glfwTerminate();
    printf("GLFW Terminated...\n");

    pthread_barrier_wait(&_exit_barrier);

    return nullptr;
}

static void field_loop(Field* restrict context) {
    pthread_mutex_init(&_screen_lock, nullptr);
    pthread_mutex_init(&_main_lock, nullptr);

    pthread_barrier_init(&_init_barrier, nullptr, 2);
    printf("Creating threads\n");

    pthread_create(&field_thread, nullptr, draw_field, context);
    pthread_create(&event_thread, nullptr, events_process, nullptr);

    pthread_barrier_destroy(&_init_barrier);

    while (_switch_on)
    {
        pthread_mutex_lock(&_main_lock);
        pthread_cond_wait(&_escape_condition, &_main_lock);
        pthread_mutex_unlock(&_main_lock);
    }

    pthread_barrier_init(&_exit_barrier, nullptr, 3);
    pthread_barrier_wait(&_exit_barrier);
    pthread_barrier_destroy(&_exit_barrier);

    printf("Main Terminated...\n");
    pthread_mutex_destroy(&_screen_lock);
    pthread_mutex_destroy(&_main_lock);

    exit(EXIT_SUCCESS);
}
