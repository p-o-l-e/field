/**
 * @file geometry.h
 * @brief Software 2-D drawing primitives — pixel buffers, sprites, shapes, text, and colour utilities.
 *
 * @details This header provides the low-level rendering toolkit used by the
 * field framework:
 * - Geometric types: @ref point, @ref point32u, @ref point32s, @ref ltrb32u,
 *   @ref ltwh32u.
 * - Pixel containers: @ref Frame (CPU-side RGBA buffer) and @ref Sprite
 *   (multi-frame animation).
 * - Colour palette (@ref Palette) and colour-space helpers
 *   (HSVA→RGBA, index→colour).
 * - Frame operations: pixel get/set, clear, fill, copy, dirty-rect tracking.
 * - Draw primitives: rectangles, lines, antialiased SDF circles, glyphs, text.
 * - Sprite operations: allocate, free, load stripe.
 * - Embedded 8×8 bitmap font (@ref gtFont, defined in fonts.h).
 *
 * The design is header-only: all functions are defined inline in this file.
 * Rendering targets are @ref Frame buffers (32-bit RGBA pixel arrays).
 *
 * @note The frame loop and graph evaluation are expected to be allocation-free;
 *       this file allocates only during frame/sprite creation and teardown.
 */

#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "fonts.h"

/** @name Colour palette
 * @brief 32-bit RGBA colour constants (0xRRGGBBAA) used throughout the renderer.
 * @{ */
typedef enum
{
    BACKGROUND          = 0x263238FF,   ///< Default canvas background.
    MINOR               = 0x232935FF,   ///< Minor grid lines.
    MAJOR               = 0x162228FF,   ///< Major grid lines.
    FOREGROUND          = 0xB0BEC5FF,   ///< Default foreground / widget body.
    TEXT                = 0x607D8BFF,   ///< Text label colour.
    SELECTIONBACKGROUND = 0x546E7AFF,   ///< Selected-item background.
    SELECTIONFOREGROUND = 0xFFFFFFFF,   ///< Selected-item foreground.
    BUTTONS             = 0x2E3C43FF,   ///< Button / widget background.
    SECONDBACKGROUND    = 0x32424AFF,   ///< Secondary background (nodes).
    DISABLED            = 0x415967FF,   ///< Disabled widget colour.
    CONTRAST            = 0x1E272CFF,   ///< Contrast / shadow colour.
    ACTIVE              = 0x314549FF,   ///< Active / pressed state.
    BORDER              = 0x2A373EFF,   ///< Widget border colour.
    HIGHLIGHT           = 0x425B67FF,   ///< Slider fill / highlight colour.
    NOTIFICATIONS       = 0x1E272CFF,   ///< Notification background.
    ACCENT              = 0x009688FF,   ///< Accent colour (teal).
    EXCLUDED            = 0x2E3C43FF,   ///< Excluded / muted element.
    GREEN               = 0xC3E88DFF,   ///< Syntax: green.
    YELLOW              = 0xFFCB6BFF,   ///< Syntax: yellow.
    BLUE                = 0x82AAFFFF,   ///< Syntax: blue.
    RED                 = 0xF07178FF,   ///< Syntax: red.
    PURPLE              = 0xC792EAFF,   ///< Syntax: purple.
    ORANGE              = 0xF78C6CFF,   ///< Syntax: orange.
    CYAN                = 0x89DDFFFF,   ///< Syntax: cyan.
    GRAY                = 0x546E7AFF,   ///< Syntax: gray.
    ERROR               = 0xFF5370FF,   ///< Error / overlap indicator.
    COMMENTS            = 0x546E7AFF,   ///< Syntax: comments.
    VARIABLES           = 0xEEFFFFFF,   ///< Syntax: variables.
    LINKS               = 0x80CBC4FF,   ///< Syntax: links.
    FUNCTIONS           = 0x82AAFFFF,   ///< Syntax: functions.
    KEYWORDS            = 0xC792EAFF,   ///< Syntax: keywords.
    TAGS                = 0xF07178FF,   ///< Syntax: tags.
    STRINGS             = 0xC3E88DFF,   ///< Syntax: strings.
    OPERATORS           = 0x89DDFFFF,   ///< Syntax: operators.
    ATTRIBUTES          = 0xFFCB6BFF,   ///< Syntax: attributes.
    NUMBERS             = 0xF78C6CFF,   ///< Syntax: numbers.
    PARAMETERS          = 0xF78C6CFF,   ///< Syntax: parameters.
    SOCKET_IN           = 0x009688FF,   ///< Input socket colour.
    SOCKET_OUT          = 0xF07178FF,   ///< Output socket colour.

} Palette;
/** @} */

/**
 * @brief Layout constraint constants (pixel offsets).
 */
typedef enum {
    GRIP    = 3,    ///< Slider grip half-height in pixels.
    GAP     = 2,    ///< Inner padding between widget border and content.
    SOCKET_BORDER = 2,  ///< Socket border thickness.

} Constrain;

/** @name Primitive types
 * @{ */

/** @brief 2-D point with float coordinates (normalised UV or screen space). */
typedef struct
{
    float x;    ///< X coordinate.
    float y;    ///< Y coordinate.

} point;

/** @brief 2-D point with unsigned 32-bit integer coordinates (screen space). */
typedef struct
{
    uint32_t x; ///< X coordinate.
    uint32_t y; ///< Y coordinate.

} point32u;

/** @brief 2-D point with signed 32-bit integer coordinates. */
typedef struct
{
    int32_t x;  ///< X coordinate.
    int32_t y;  ///< Y coordinate.

} point32s;

/**
 * @brief Axis-aligned bounding box in screen coordinates (left, top, right, bottom).
 * @note @c r and @c b are inclusive pixel indices.
 */
typedef struct
{
    uint32_t l; ///< Left edge.
    uint32_t t; ///< Top edge.
    uint32_t r; ///< Right edge (inclusive).
    uint32_t b; ///< Bottom edge (inclusive).

} ltrb32u;

/**
 * @brief Rectangle defined by left, top, width, height.
 */
typedef struct
{
    uint32_t l; ///< Left edge.
    uint32_t t; ///< Top edge.
    uint32_t w; ///< Width in pixels.
    uint32_t h; ///< Height in pixels.

} ltwh32u;

/**
 * @brief A 2-D pixel buffer of 32-bit RGBA values.
 * @details Pixels are stored row-major: @c data[x + y * width].
 *          Use ff_frame_init() / ff_frame_flush() for allocation.
 */
typedef struct
{
    uint32_t* data;     ///< Pixel array (capacity entries; only width × height are used).
    uint32_t width;     ///< Buffer width in pixels.
    uint32_t height;    ///< Buffer height in pixels.
    size_t   capacity;  ///< Allocated pixels (>= width × height); grows on resize, never shrinks.
    ltrb32u  dirty;     ///< Union of modified regions since the last reset (inclusive bounds).
    bool     is_dirty;  ///< Whether @c dirty holds a valid region (false = nothing modified).

} Frame;

/**
 * @brief A sprite — an array of @ref Frame objects played back as animation frames.
 */
typedef struct
{
    Frame*   data;      ///< Array of @c nframes frames.
    uint32_t width;     ///< Frame width in pixels.
    uint32_t height;    ///< Frame height in pixels.
    uint32_t nframes;   ///< Number of frames in the sprite.

} Sprite;
/** @} */

/** @name Function prototypes
 * @{ */

/** @brief Extract a single byte from a 32-bit value.
 *  @param value  The 32-bit source value.
 *  @param byte   Byte index (0 = LSB, 3 = MSB).
 *  @return The extracted 8-bit value.
 */
[[nodiscard]] uint8_t     ff_extract_byte(uint32_t, uint_fast8_t) [[reproducible]];

/** @brief Convert normalised UV coordinates to screen pixel coordinates.
 *  @param x  Normalised X (0.0–1.0).
 *  @param y  Normalised Y (0.0–1.0, top-down).
 *  @param w  Target width in pixels.
 *  @param h  Target height in pixels.
 *  @return Screen-space pixel coordinates.
 */
[[nodiscard]] point32u    ff_uv_to_screen(float, float, uint32_t, uint32_t) [[reproducible]];

/** @brief Convert screen pixel coordinates to normalised UV coordinates.
 *  @param x  Screen X pixel.
 *  @param y  Screen Y pixel (top-down).
 *  @param w  Source width in pixels.
 *  @param h  Source height in pixels.
 *  @return Normalised UV coordinates (Y flipped: 0 at bottom).
 */
[[nodiscard]] point       ff_screen_to_uv(uint32_t, uint32_t, uint32_t, uint32_t) [[reproducible]];

/** @brief Convert HSVA (0–255 each) to a packed 32-bit RGBA value.
 *  @param h  Hue (0–255).
 *  @param s  Saturation (0–255).
 *  @param v  Value / brightness (0–255).
 *  @param a  Alpha (0–255).
 *  @return Packed 0xRRGGBBAA colour.
 */
[[nodiscard]] static uint32_t ff_hsva_to_rgba(uint8_t, uint8_t, uint8_t, uint8_t) [[reproducible]];

/** @brief Generate a deterministic colour from an index (golden-ratio hue spread).
 *  @param index  Unique index (e.g. entity index).
 *  @param alpha  Alpha value (0–255).
 *  @return Packed 0xRRGGBBAA colour.
 */
[[nodiscard]] uint32_t    ff_index_to_hsv(uint32_t, uint8_t);

/** @brief Set a single pixel in a frame (bounds-checked).
 *  @param frame  Target frame.
 *  @param x      Pixel X.
 *  @param y      Pixel Y.
 *  @param value  32-bit RGBA value.
 */
static void     ff_frame_set(Frame* restrict, uint32_t, uint32_t, uint32_t);

/** @brief Get a single pixel from a frame (bounds-checked, returns pixel 0 on OOB).
 *  @param frame  Source frame.
 *  @param x      Pixel X.
 *  @param y      Pixel Y.
 *  @return 32-bit RGBA value.
 */
[[nodiscard]] static uint32_t ff_frame_get(const Frame* restrict, uint32_t, uint32_t);

/** @brief Clear the entire frame to zero (transparent black).
 *  @param frame  Target frame.
 */
static void     ff_frame_clr(Frame* restrict);

/** @brief Fill the entire frame with a single colour.
 *  @param frame  Target frame.
 *  @param value  32-bit RGBA fill value.
 */
static void     ff_frame_fill(Frame* restrict, uint32_t);

/** @brief Allocate a frame buffer of the given dimensions (exact capacity).
 *  @param frame  Frame to initialise.
 *  @param x      Width in pixels.
 *  @param y      Height in pixels.
 */
static void     ff_frame_init(Frame* restrict, uint32_t, uint32_t);

/** @brief Allocate a frame buffer of the given dimensions with explicit capacity.
 *  @details Window/node resize heaps pass 2x the window size here: resizes
 *           within the headroom never reallocate (see ff_frame_resize).
 *  @param frame     Frame to initialise.
 *  @param x         Width in pixels.
 *  @param y         Height in pixels.
 *  @param capacity  Allocated pixel count (must be >= x × y).
 */
static void     ff_frame_init_cap(Frame* restrict, uint32_t, uint32_t, size_t);

/** @brief Resize a frame buffer, preserving the overlapping region.
 *  @details Heap policy: the pixel array is allocated at the owning field's
 *           heap_init_mult × the used size at creation; when a resize
 *           outgrows the heap it grows by +grow_num/grow_den of its capacity
 *           per step until it fits; shrinking NEVER reallocates — the slack
 *           stays allocated and is reused when the window grows back. The
 *           used region is re-packed in place (the row stride changes with
 *           the width): rows are moved with memmove (overlap-safe within a
 *           row) top-down when narrowing and bottom-up when widening, so no
 *           row is overwritten before it is read. The grown area is
 *           zero-filled. The whole frame is marked dirty so the backend
 *           re-uploads it.
 *  @param frame     Frame to resize.
 *  @param w         New width in pixels.
 *  @param h         New height in pixels.
 *  @param grow_num  Grow-step numerator (must be >= 1).
 *  @param grow_den  Grow-step denominator (must be >= 1).
 */
static void     ff_frame_resize(Frame* restrict, uint32_t, uint32_t, uint32_t, uint32_t);

/** @brief Free a frame's pixel buffer.
 *  @param frame  Frame to release.
 */
static void     ff_frame_flush(Frame* restrict);

/** @brief Copy all pixels from source to target (dimensions must match).
 *  @param target  Destination frame.
 *  @param source  Source frame (read-only).
 */
static void     ff_frame_copy(Frame* restrict target, const Frame* restrict source);

/** @brief Copy source to target, recoloring each non-zero pixel via ff_index_to_hsv.
 *  @param target  Destination frame.
 *  @param source  Source frame (read-only, used as stencil index).
 *  @param alpha   Alpha value for the recolored output.
 */
static void     ff_frame_copy_with_alpha(Frame* restrict target, const Frame* restrict source, uint8_t);

/** @brief Copy source to target at a pixel offset, skipping zero (transparent) pixels.
 *  @param target  Destination frame.
 *  @param source  Source frame (read-only).
 *  @param xo      X offset in target.
 *  @param yo      Y offset in target.
 */
static void     ff_frame_copy_at(Frame* restrict target, const Frame* restrict source, uint32_t, uint32_t);

/** @brief Union a modified region into a frame's dirty rectangle (clamped to bounds).
 *  @param frame  Target frame.
 *  @param l      Left edge (inclusive; negative values are clamped to 0).
 *  @param t      Top edge (inclusive; negative values are clamped to 0).
 *  @param r      Right edge (inclusive).
 *  @param b      Bottom edge (inclusive).
 */
static void     ff_frame_mark(Frame* restrict, int32_t, int32_t, int32_t, int32_t);

/** @brief Reset a frame's dirty rectangle (call after the region was uploaded).
 *  @param frame  Target frame.
 */
static void     ff_frame_dirty_reset(Frame* restrict);

/** @brief Allocate a sprite with @c nframes frames of size @c w × @c h.
 *  @param sprite   Sprite to initialise.
 *  @param w        Frame width in pixels.
 *  @param h        Frame height in pixels.
 *  @param nframes  Number of frames.
 */
static void ff_sprite_init(Sprite* restrict, uint32_t, uint32_t, uint32_t);

/** @brief Free all frames and the sprite's frame array.
 *  @param sprite  Sprite to release.
 */
static void ff_sprite_flush(Sprite* restrict);

/** @brief Load a horizontal stripe image into sprite frames.
 *  @details The source frame must be @c w × (h * nframes) pixels;
 *           each frame is extracted from a vertical slice.
 *  @param sprite  Target sprite.
 *  @param frame   Source stripe image (read-only).
 */
static void ff_sprite_load_stripe(Sprite* restrict, const Frame* restrict);

/** @brief Draw a single glyph from a bitmap font.
 *  @param frame  Target frame.
 *  @param font   Font bitmap (size bytes per glyph, row-major).
 *  @param size   Glyph width/height in pixels (≤ 8, one byte per row).
 *  @param id     Glyph index.
 *  @param l      Left pixel position.
 *  @param t      Top pixel position.
 *  @param colour 32-bit RGBA colour.
 */
static void ff_draw_glyph(Frame* restrict, const uint8_t* restrict, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);

/** @brief Draw a text string using the embedded bitmap font.
 *  @param frame  Target frame.
 *  @param font   Font bitmap.
 *  @param size   Glyph width/height in pixels (character advance).
 *  @param text   Null-terminated string (ASCII 32–127).
 *  @param l      Left pixel position.
 *  @param t      Top pixel position.
 *  @param _w     Unused (reserved).
 *  @param _h     Unused (reserved).
 *  @param colour 32-bit RGBA colour.
 */
static void ffDrawTextLabel(Frame* restrict, const uint8_t* restrict, uint32_t, const char* restrict, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);

/** @brief Draw a rectangle outline (1-pixel border).
 *  @param frame  Target frame.
 *  @param l      Left edge.
 *  @param t      Top edge.
 *  @param r      Right edge.
 *  @param b      Bottom edge.
 *  @param colour 32-bit RGBA colour.
 */
static void ff_draw_rect_o(Frame* restrict, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);

/** @brief Draw a filled rectangle.
 *  @param frame  Target frame.
 *  @param l      Left edge.
 *  @param t      Top edge.
 *  @param r      Right edge.
 *  @param b      Bottom edge.
 *  @param colour 32-bit RGBA colour.
 */
static void ff_draw_rect_f(Frame* restrict, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);

/** @brief Draw a vertical line.
 *  @param frame  Target frame.
 *  @param x      X column.
 *  @param yo     Start Y (inclusive).
 *  @param ye     End Y (inclusive).
 *  @param value  32-bit RGBA colour.
 */
static void ff_draw_line_v(Frame* restrict, uint32_t, uint32_t, uint32_t, const uint32_t);

/** @brief Draw a horizontal line.
 *  @param frame  Target frame.
 *  @param y      Y row.
 *  @param xo     Start X (inclusive).
 *  @param xe     End X (inclusive).
 *  @param value  32-bit RGBA colour.
 */
static void ff_draw_line_h(Frame* restrict, uint32_t, uint32_t, uint32_t, const uint32_t);

/** @brief Alpha-blend a source colour over a destination pixel by coverage.
 *  @param dst       Destination pixel value (0xRRGGBBAA).
 *  @param src       Source colour (0xRRGGBBAA).
 *  @param coverage  Blend factor (0.0 = fully dst, 1.0 = fully src).
 *  @return Blended pixel value (0xRRGGBBAA).
 *  @note Assumes source is opaque (alpha = 0xFF); result alpha is 0xFF.
 *        All palette colours are opaque, so this covers every UI use case.
 */
[[nodiscard]] static inline uint32_t ff_blend_coverage(uint32_t dst, uint32_t src, float coverage) [[reproducible]];

/** @brief Draw a filled antialiased circle (analytic SDF coverage).
 *  @param frame  Target frame.
 *  @param xc     Centre X (sub-pixel precision).
 *  @param yc     Centre Y (sub-pixel precision).
 *  @param r      Radius in pixels.
 *  @param value  32-bit RGBA colour.
 *  @note Uses a signed-distance field: interior pixels are written directly
 *        (squared-distance early-out, no sqrt); only the ~2πr boundary pixels
 *        compute sqrtf + alpha blend for perfect box-filtered antialiasing.
 */
static void ff_draw_sdf_circle_fill(Frame* restrict, float, float, float, uint32_t);

/** @brief Draw an antialiased circle outline (analytic SDF ring).
 *  @param frame     Target frame.
 *  @param xc        Centre X (sub-pixel precision).
 *  @param yc        Centre Y (sub-pixel precision).
 *  @param r         Radius to the ring centreline in pixels.
 *  @param thickness Stroke thickness in pixels (1.0 = hairline).
 *  @param value     32-bit RGBA colour.
 *  @note Same SDF approach as ff_draw_sdf_circle_fill; coverage falls off linearly
 *        across the ring band. Pixels in the hole (dist < r - thickness/2)
 *        and outside the outer edge are skipped via squared-distance tests.
 */
static void ff_draw_sdf_circle_stroke(Frame* restrict, float, float, float, float, uint32_t);

/** @brief Draw a bounding-box outline from an @ref ltrb32u.
 *  @param canvas  Target frame.
 *  @param r       Bounding box (read-only).
 *  @param colour  32-bit RGBA colour.
 */
static void ff_draw_ltrb_o(Frame* restrict, const ltrb32u* restrict, const uint32_t);

/** @brief Draw a filled bounding box from an @ref ltrb32u.
 *  @param canvas  Target frame.
 *  @param r       Bounding box (read-only).
 *  @param colour  32-bit RGBA colour.
 */
static void ff_draw_ltrb_f(Frame* restrict, const ltrb32u* restrict, const uint32_t);

/** @} */

/** Color & Utility ***********************************************************************************************************/

/**
 * @brief Extract a single byte from a 32-bit value.
 * @details Data layout: 0x FF FF FF FF, index 3 2 1 0 (MSB→LSB).
 * @param value  The 32-bit source value.
 * @param byte   Byte index (0 = LSB, 3 = MSB).
 * @return The extracted 8-bit value.
 */
[[nodiscard]] uint8_t ff_extract_byte(uint32_t value, uint_fast8_t byte) [[reproducible]] {
    return (value >> (byte * 8)) & 0xFF;
}

/** @brief Convert normalised UV coordinates to screen pixel coordinates.
 *  @param x  Normalised X (0.0–1.0).
 *  @param y  Normalised Y (0.0–1.0, top-down).
 *  @param w  Target width in pixels.
 *  @param h  Target height in pixels.
 *  @return Screen-space pixel coordinates.
 */
[[nodiscard]] point32u ff_uv_to_screen(float x, float y, uint32_t w, uint32_t h) [[reproducible]] {
    point32u s = {
        .x = (uint32_t)(x * (float)w),
        .y = (uint32_t)(y * (float)h)
    };
    return s;
}

/** @brief Convert screen pixel coordinates to normalised UV coordinates (Y flipped).
 *  @param x  Screen X pixel.
 *  @param y  Screen Y pixel (top-down).
 *  @param w  Source width in pixels.
 *  @param h  Source height in pixels.
 *  @return Normalised UV coordinates (Y: 0 at bottom, 1 at top).
 */
[[nodiscard]] point ff_screen_to_uv(uint32_t x, uint32_t y, uint32_t w, uint32_t h) [[reproducible]] {
    point n = {
        .x =  (float)x / (float)w,
        .y =  1.0f - (float)y / (float)h
    };
    return n;
}

/** @brief Convert HSVA (0–255 each) to a packed 32-bit RGBA value.
 *  @param h  Hue (0–255, wraps at 256).
 *  @param s  Saturation (0–255).
 *  @param v  Value / brightness (0–255).
 *  @param a  Alpha (0–255).
 *  @return Packed 0xRRGGBBAA colour.
 */
[[nodiscard]] static uint32_t ff_hsva_to_rgba(uint8_t h, uint8_t s, uint8_t v, uint8_t a) [[reproducible]] {
    const uint8_t region = h / 43;
    const uint8_t remainder = (h - region * 43) * 6;

    const uint8_t p = (v * (255 - s)) >> 8;
    const uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    const uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    uint8_t r, g, b;
    switch (region) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    return (r << 24) | (g << 16) | (b << 8) | a;
}

/** @brief Generate a deterministic colour from an index (golden-ratio hue spread).
 *  @param index  Unique index (e.g. entity index for stencil colouring).
 *  @param alpha  Alpha value (0–255).
 *  @return Packed 0xRRGGBBAA colour.
 */
[[nodiscard]] uint32_t ff_index_to_hsv(uint32_t index, uint8_t alpha) {
    const uint8_t hue = (index * 137) & 0xFF;
    constexpr uint8_t sat = 204;
    constexpr uint8_t val = 230;
    return ff_hsva_to_rgba(hue, sat, val, alpha);
}

/** Frame Operations **********************************************************************************************************/

/** @brief Set a single pixel in a frame (bounds-checked, no-op if out of range).
 *  @param frame  Target frame.
 *  @param x      Pixel X coordinate.
 *  @param y      Pixel Y coordinate.
 *  @param value  32-bit RGBA pixel value.
 */
static inline void ff_frame_set(Frame* restrict frame, uint32_t x, uint32_t y, uint32_t value) {
    if(x < frame->width && y < frame->height)
    frame->data[x + y * frame->width] = value;
}

/** @brief Get a single pixel from a frame (bounds-checked, returns pixel [0,0] if OOB).
 *  @param frame  Source frame.
 *  @param x      Pixel X coordinate.
 *  @param y      Pixel Y coordinate.
 *  @return 32-bit RGBA pixel value.
 */
[[nodiscard]] static inline uint32_t ff_frame_get(const Frame* restrict frame, uint32_t x, uint32_t y) {
    if(x < frame->width && y < frame->height) return frame->data[x + y * frame->width];
    return frame->data[0];
}

/** @brief Clear the entire frame buffer to zero (transparent black).
 *  @param frame  Target frame.
 */
static inline void ff_frame_clr(Frame* restrict frame) {
    memset(frame->data, 0, frame->height * frame->width * sizeof(uint32_t));
    ff_frame_mark(frame, 0, 0, (int32_t)frame->width - 1, (int32_t)frame->height - 1);
}

/** @brief Fill the entire frame with a single colour value.
 *  @param frame  Target frame.
 *  @param value  32-bit RGBA fill value.
 */
static inline void ff_frame_fill(Frame* restrict frame, uint32_t value) {
    for(uint32_t i = 0; i < (frame->height * frame->width); ++i) frame->data[i] = value;
    ff_frame_mark(frame, 0, 0, (int32_t)frame->width - 1, (int32_t)frame->height - 1);
}

/** @brief Allocate and zero-initialise a frame buffer (exact capacity).
 *  @param frame  Frame to initialise.
 *  @param x      Width in pixels.
 *  @param y      Height in pixels.
 */
static inline void ff_frame_init(Frame* restrict frame, uint32_t x, uint32_t y) {
    ff_frame_init_cap(frame, x, y, (size_t)x * y);
}

/** @brief Allocate and zero-initialise a frame buffer with explicit capacity.
 *  @param frame     Frame to initialise.
 *  @param x         Width in pixels.
 *  @param y         Height in pixels.
 *  @param capacity  Allocated pixel count (must be >= x × y).
 */
static inline void ff_frame_init_cap(Frame* restrict frame, uint32_t x, uint32_t y, size_t capacity) {
    frame->width     = x;
    frame->height    = y;
    frame->capacity  = capacity;
    frame->data      = (uint32_t*)calloc(capacity, sizeof(uint32_t));
    frame->is_dirty  = false;
}

/** @brief Resize a frame buffer, preserving the overlapping top-left region.
 *  @details Heap policy: the array grows by +grow_num/grow_den of its capacity
 *           per step when the new size outgrows it, and is never reallocated
 *           on shrink — the slack stays allocated and is reused on the next
 *           grow. The used region is re-packed in place because the row
 *           stride changes with the width: memmove is overlap-safe within a
 *           row, and the row order is chosen so no row is overwritten before
 *           it is read (top-down when narrowing, bottom-up when widening).
 *           Growing zero-fills, shrinking crops. The full frame is marked
 *           dirty so the backend re-uploads it.
 *  @param frame     Frame to resize.
 *  @param w         New width in pixels.
 *  @param h         New height in pixels.
 *  @param grow_num  Grow-step numerator (must be >= 1).
 *  @param grow_den  Grow-step denominator (must be >= 1).
 */
static inline void ff_frame_resize(Frame* restrict frame, uint32_t w, uint32_t h,
                                   uint32_t grow_num, uint32_t grow_den) {
    if(w == 0 || h == 0) return;
    if(w == frame->width && h == frame->height) return;

    const uint32_t old_w = frame->width;
    const uint32_t old_h = frame->height;
    const size_t   needed = (size_t)w * h;

    if(needed > frame->capacity) {
        size_t capacity = frame->capacity;
        /* The +num/den steps only make progress once capacity >= den/num —
         * seed the denominator so a frame created zero-sized (an anchored
         * node whose width is only set by this first resize) still grows. */
        if(capacity < grow_den) capacity = grow_den;
        do { capacity += (size_t)capacity * grow_num / grow_den; } while(capacity < needed);
        frame->data     = (uint32_t*)realloc(frame->data, capacity * sizeof(uint32_t));
        frame->capacity = capacity;
    }

    const uint32_t copy_w = w < old_w ? w : old_w;
    const uint32_t copy_h = h < old_h ? h : old_h;

    if(w > old_w) {
        /* Widening: re-pack bottom-up so a row's new location never clobbers
         * a row that is still to be read, then zero the grown area (right
         * strip of the kept rows plus any new bottom rows). copy_w == 0 means
         * the frame had no width — nothing to re-pack, and the zero-fill
         * below already covers every row. */
        if(copy_w)
            for(uint32_t y = copy_h; y-- > 0;)
                memmove(frame->data + (size_t)y * w, frame->data + (size_t)y * old_w,
                        (size_t)copy_w * sizeof(uint32_t));
        for(uint32_t y = 0; y < copy_h; ++y)
            memset(frame->data + (size_t)y * w + copy_w, 0,
                   (size_t)(w - copy_w) * sizeof(uint32_t));
        for(uint32_t y = copy_h; y < h; ++y)
            memset(frame->data + (size_t)y * w, 0, (size_t)w * sizeof(uint32_t));
    }
    else {
        /* Narrowing (or same width, which needs no re-pack): top-down. Zero
         * any new bottom rows when the height grew. */
        if(w < old_w)
            for(uint32_t y = 0; y < copy_h; ++y)
                memmove(frame->data + (size_t)y * w, frame->data + (size_t)y * old_w,
                        (size_t)copy_w * sizeof(uint32_t));
        for(uint32_t y = copy_h; y < h; ++y)
            memset(frame->data + (size_t)y * w, 0, (size_t)w * sizeof(uint32_t));
    }

    frame->width  = w;
    frame->height = h;
    ff_frame_mark(frame, 0, 0, (int32_t)w - 1, (int32_t)h - 1);
}

/** @brief Free the frame's pixel buffer (does not free the Frame struct itself).
 *  @param frame  Frame whose data to free.
 */
static inline void ff_frame_flush(Frame* restrict frame) {
    free(frame->data);
}

/** @brief Union a modified region into a frame's dirty rectangle.
 *  @details The rectangle is clamped to the frame bounds; empty or fully
 *           out-of-range rectangles are ignored. The draw primitives call
 *           this so the backend can upload only the changed region
 *           (dirty-rect texturing) instead of the whole layer. Cost is a
 *           few integer compares per primitive — deliberately NOT per pixel
 *           in ff_frame_set(), which stays mark-free.
 *  @param frame  Target frame.
 *  @param l      Left edge (inclusive).
 *  @param t      Top edge (inclusive).
 *  @param r      Right edge (inclusive).
 *  @param b      Bottom edge (inclusive).
 */
static inline void ff_frame_mark(Frame* restrict frame, int32_t l, int32_t t, int32_t r, int32_t b) {
    if(r < l || b < t) return;
    if(l < 0) l = 0;
    if(t < 0) t = 0;
    if(r >= (int32_t)frame->width)  r = (int32_t)frame->width - 1;
    if(b >= (int32_t)frame->height) b = (int32_t)frame->height - 1;
    if(l > r || t > b) return;

    if(!frame->is_dirty) {
        frame->dirty.l   = (uint32_t)l;
        frame->dirty.t   = (uint32_t)t;
        frame->dirty.r   = (uint32_t)r;
        frame->dirty.b   = (uint32_t)b;
        frame->is_dirty = true;
        return;
    }
    if((uint32_t)l < frame->dirty.l) frame->dirty.l = (uint32_t)l;
    if((uint32_t)t < frame->dirty.t) frame->dirty.t = (uint32_t)t;
    if((uint32_t)r > frame->dirty.r) frame->dirty.r = (uint32_t)r;
    if((uint32_t)b > frame->dirty.b) frame->dirty.b = (uint32_t)b;
}

/** @brief Reset a frame's dirty rectangle (call after the region was uploaded).
 *  @param frame  Target frame.
 */
static inline void ff_frame_dirty_reset(Frame* restrict frame) {
    frame->is_dirty = false;
}

/** @brief Copy all pixels from source to target (dimensions must match).
 *  @param target  Destination frame.
 *  @param source  Source frame.
 */
static inline void ff_frame_copy(Frame* restrict target, const Frame* restrict source) {
    for(uint32_t i = 0; i < target->height * target->width; i++) target->data[i] = source->data[i];
    ff_frame_mark(target, 0, 0, (int32_t)target->width - 1, (int32_t)target->height - 1);
}

/** @brief Copy source to target, recoloring each pixel via ff_index_to_hsv.
 *  @param target  Destination frame.
 *  @param source  Source frame (used as stencil index).
 *  @param alpha   Alpha value for the recolored output.
 */
static void ff_frame_copy_with_alpha(Frame* restrict target, const Frame* restrict source, uint8_t alpha) {
    for(uint32_t i = 0; i < target->height * target->width; ++i) {
        auto data = ff_index_to_hsv(source->data[i] + 0x2, alpha);
        target->data[i] = data;
    }
    ff_frame_mark(target, 0, 0, (int32_t)target->width - 1, (int32_t)target->height - 1);
}

/** @brief Copy source to target at a pixel offset, skipping transparent (zero) pixels.
 *  @param target  Destination frame.
 *  @param source  Source frame.
 *  @param xo      X offset in target.
 *  @param yo      Y offset in target.
 */
static inline void ff_frame_copy_at(Frame* restrict target, const Frame* restrict source, uint32_t xo, uint32_t yo) {
    ff_frame_mark(target, (int32_t)xo, (int32_t)yo,
                  (int32_t)(xo + source->width - 1),
                  (int32_t)(yo + source->height - 1));
    for(uint32_t y = 0; y < source->height; y++) {
        for(uint32_t x = 0; x < source->width; x++) {
            auto data = ff_frame_get(source, x, y);
            if(data)
                ff_frame_set(target, x + xo, y + yo, data);
        }
    }
}

/** Draw Primitives ***********************************************************************************************************/

/** @brief Draw a rectangle outline (1-pixel border).
 *  @param frame   Target frame.
 *  @param l       Left edge.
 *  @param t       Top edge.
 *  @param r       Right edge (inclusive).
 *  @param b       Bottom edge (inclusive).
 *  @param colour  32-bit RGBA colour.
 */
static inline void ff_draw_rect_o(Frame* restrict frame, uint32_t l, uint32_t t, uint32_t r, uint32_t b, const uint32_t colour) {
    ff_frame_mark(frame, (int32_t)l, (int32_t)t, (int32_t)r, (int32_t)b);
    for(uint32_t i = l; i <= r; i++) {
        ff_frame_set(frame, i, t, colour);
        ff_frame_set(frame, i, b, colour);
    }

    for(uint32_t i = t; i <= b; i++) {
        ff_frame_set(frame, l, i, colour);
        ff_frame_set(frame, r, i, colour);
    }
}

/** @brief Draw a filled rectangle.
 *  @param frame   Target frame.
 *  @param l       Left edge.
 *  @param t       Top edge.
 *  @param r       Right edge (inclusive).
 *  @param b       Bottom edge (inclusive).
 *  @param colour  32-bit RGBA colour.
 */
static inline void ff_draw_rect_f(Frame* restrict frame, uint32_t l, uint32_t t, uint32_t r, uint32_t b, const uint32_t colour) {
    ff_frame_mark(frame, (int32_t)l, (int32_t)t, (int32_t)r, (int32_t)b);
    for(uint32_t y = t; y <= b; ++y) {
        for(uint32_t x = l; x <= r; ++x) {
            ff_frame_set(frame, x, y, colour);
        }
    }
}

/** @brief Draw a vertical line.
 *  @param frame  Target frame.
 *  @param x      X column.
 *  @param yo     Start Y (inclusive).
 *  @param ye     End Y (inclusive).
 *  @param value  32-bit RGBA colour.
 */
static inline void ff_draw_line_v(Frame* restrict frame, uint32_t x, uint32_t yo, uint32_t ye, const uint32_t value) {
    ff_frame_mark(frame, (int32_t)x, (int32_t)yo, (int32_t)x, (int32_t)ye);
    for(uint32_t y = yo; y <= ye; ++y)
        ff_frame_set(frame, x, y, value);
}

/** @brief Draw a horizontal line.
 *  @param frame  Target frame.
 *  @param y      Y row.
 *  @param xo     Start X (inclusive).
 *  @param xe     End X (inclusive).
 *  @param value  32-bit RGBA colour.
 */
static inline void ff_draw_line_h(Frame* restrict frame, uint32_t y, uint32_t xo, uint32_t xe, const uint32_t value) {
    ff_frame_mark(frame, (int32_t)xo, (int32_t)y, (int32_t)xe, (int32_t)y);
    for(uint32_t x = xo; x <= xe; ++x)
        ff_frame_set(frame, x, y, value);
}

/** @brief Alpha-blend a source colour over a destination pixel by coverage.
 *  @param dst       Destination pixel value (0xRRGGBBAA).
 *  @param src       Source colour (0xRRGGBBAA).
 *  @param coverage  Blend factor (0.0 = fully dst, 1.0 = fully src).
 *  @return Blended pixel value (0xRRGGBBAA).
 *  @note Assumes source is opaque (alpha = 0xFF); result alpha is 0xFF.
 *        All palette colours are opaque, so this covers every UI use case.
 */
[[nodiscard]] static inline uint32_t ff_blend_coverage(uint32_t dst, uint32_t src, float coverage) [[reproducible]] {
    const float inv = 1.0f - coverage;
    // Unpack RGB channels (pixel format: 0xRRGGBBAA)
    const float sr = (float)((src >> 24) & 0xFFu);
    const float sg = (float)((src >> 16) & 0xFFu);
    const float sb = (float)((src >>  8) & 0xFFu);
    const float dr = (float)((dst >> 24) & 0xFFu);
    const float dg = (float)((dst >> 16) & 0xFFu);
    const float db = (float)((dst >>  8) & 0xFFu);
    // Linear blend per channel, repack with opaque alpha
    const uint32_t r = (uint32_t)(sr * coverage + dr * inv);
    const uint32_t g = (uint32_t)(sg * coverage + dg * inv);
    const uint32_t b = (uint32_t)(sb * coverage + db * inv);
    return (r << 24) | (g << 16) | (b << 8) | 0xFFu;
}

/** @brief Draw a filled antialiased circle (analytic SDF coverage).
 *  @param frame  Target frame.
 *  @param xc     Centre X (sub-pixel precision).
 *  @param yc     Centre Y (sub-pixel precision).
 *  @param r      Radius in pixels.
 *  @param value  32-bit RGBA colour.
 *  @note Uses a signed-distance field: interior pixels are written directly
 *        (squared-distance early-out, no sqrt); only the ~2πr boundary pixels
 *        compute sqrtf + alpha blend for perfect box-filtered antialiasing.
 */
static void ff_draw_sdf_circle_fill(Frame* restrict frame, float xc, float yc, float r, uint32_t value) {
    if (r <= 0.0f) return;

    // Bounding box with 1px margin for the AA fringe
    const int32_t l  = (int32_t)floorf(xc - r - 1.0f);
    const int32_t t  = (int32_t)floorf(yc - r - 1.0f);
    const int32_t rr = (int32_t)ceilf (xc + r + 1.0f);
    const int32_t b  = (int32_t)ceilf (yc + r + 1.0f);

    ff_frame_mark(frame, l, t, rr, b);

    // Squared radii for sqrt-free early-out:
    //   dist <= r_inner  →  full coverage (direct write, no blend)
    //   dist >= r_outer  →  zero coverage (skip)
    const float r_inner    = r - 0.5f;
    const float r_outer    = r + 0.5f;
    const float r_inner_sq = r_inner * r_inner;
    const float r_outer_sq = r_outer * r_outer;

    // Clamp scan region to frame bounds — eliminates per-pixel bounds checks
    const int32_t x0 = l  < 0 ? 0 : l;
    const int32_t x1 = rr >= (int32_t)frame->width  ? (int32_t)frame->width  - 1 : rr;
    const int32_t y0 = t  < 0 ? 0 : t;
    const int32_t y1 = b  >= (int32_t)frame->height ? (int32_t)frame->height - 1 : b;

    const uint32_t w = frame->width;

    for (int32_t py = y0; py <= y1; ++py) {
        const float dy    = (float)py + 0.5f - yc;  // sample at pixel centre
        const float dy_sq = dy * dy;
        uint32_t* restrict row = frame->data + (uint32_t)py * w;

        for (int32_t px = x0; px <= x1; ++px) {
            const float dx   = (float)px + 0.5f - xc;
            const float d_sq = dx * dx + dy_sq;

            if (d_sq >= r_outer_sq) continue;       // outside the AA fringe

            if (d_sq <= r_inner_sq) {
                row[px] = value;                    // fully covered — direct write
            } else {
                // Boundary pixel — sqrt + blend (~2πr pixels total)
                const float dist     = sqrtf(d_sq);
                const float coverage = r_outer - dist;  // linear falloff 0..1
                row[px] = ff_blend_coverage(row[px], value, coverage);
            }
        }
    }
}

/** @brief Draw an antialiased circle outline (analytic SDF ring).
 *  @param frame     Target frame.
 *  @param xc        Centre X (sub-pixel precision).
 *  @param yc        Centre Y (sub-pixel precision).
 *  @param r         Radius to the ring centreline in pixels.
 *  @param thickness Stroke thickness in pixels (1.0 = hairline).
 *  @param value     32-bit RGBA colour.
 *  @note Same SDF approach as ff_draw_sdf_circle_fill; coverage falls off linearly
 *        across the ring band. Pixels in the hole (dist < r - thickness/2)
 *        and outside the outer edge are skipped via squared-distance tests.
 */
static inline void ff_draw_sdf_circle_stroke(Frame* restrict frame, float xc, float yc, float r, float thickness, uint32_t value) {
    if (thickness <= 0.0f) return;

    const float half_t = thickness * 0.5f;

    // Bounding box with 1px margin for the AA fringe
    const int32_t l  = (int32_t)floorf(xc - r - half_t - 1.0f);
    const int32_t t  = (int32_t)floorf(yc - r - half_t - 1.0f);
    const int32_t rr = (int32_t)ceilf (xc + r + half_t + 1.0f);
    const int32_t b  = (int32_t)ceilf (yc + r + half_t + 1.0f);

    ff_frame_mark(frame, l, t, rr, b);

    // Squared radii for sqrt-free early-out:
    //   dist >= r_outer  →  outside the ring (skip)
    //   dist <= r_inner  →  inside the hole (skip); r_inner < 0 means no hole
    const float r_outer    = r + half_t + 0.5f;
    const float r_outer_sq = r_outer * r_outer;
    const float r_inner    = r - half_t - 0.5f;
    const float r_inner_sq = r_inner > 0.0f ? r_inner * r_inner : -1.0f;

    // Clamp scan region to frame bounds
    const int32_t x0 = l  < 0 ? 0 : l;
    const int32_t x1 = rr >= (int32_t)frame->width  ? (int32_t)frame->width  - 1 : rr;
    const int32_t y0 = t  < 0 ? 0 : t;
    const int32_t y1 = b  >= (int32_t)frame->height ? (int32_t)frame->height - 1 : b;

    const uint32_t w = frame->width;

    for (int32_t py = y0; py <= y1; ++py) {
        const float dy    = (float)py + 0.5f - yc;
        const float dy_sq = dy * dy;
        uint32_t* restrict row = frame->data + (uint32_t)py * w;

        for (int32_t px = x0; px <= x1; ++px) {
            const float dx   = (float)px + 0.5f - xc;
            const float d_sq = dx * dx + dy_sq;

            if (d_sq >= r_outer_sq) continue;       // outside the ring
            if (d_sq <= r_inner_sq) continue;       // inside the hole

            // Ring SDF: linear falloff from the centreline
            const float dist     = sqrtf(d_sq);
            const float coverage = half_t + 0.5f - fabsf(dist - r);
            if (coverage <= 0.0f) continue;

            if (coverage >= 1.0f) {
                row[px] = value;                    // fully covered
            } else {
                row[px] = ff_blend_coverage(row[px], value, coverage);
            }
        }
    }
}

/** @brief Draw a single glyph from a bitmap font.
 *  @param frame   Target frame.
 *  @param font    Font bitmap (size bytes per glyph, row-major: MSB = leftmost pixel).
 *  @param size    Glyph width/height in pixels (≤ 8, one byte per row).
 *  @param id      Glyph index.
 *  @param l       Left pixel position.
 *  @param t       Top pixel position.
 *  @param colour  32-bit RGBA colour.
 *  @details Fast path: when the glyph is fully on-screen, writes go directly
 *           to @c frame->data with a hoisted row pointer — no per-pixel bounds
 *           checks or address multiplies. Empty rows are skipped entirely.
 *           Clipped glyphs fall back to bounds-checked @ref ff_frame_set.
 */
static inline void ff_draw_glyph(Frame* restrict frame, const uint8_t* restrict font, uint32_t size, uint32_t id, uint32_t l, uint32_t t, const uint32_t colour) {
    ff_frame_mark(frame, (int32_t)l, (int32_t)t, (int32_t)(l + size - 1), (int32_t)(t + size - 1));
    const uint8_t* glyph = font + id * size;

    /* Fast path: glyph fully on-screen — direct writes, no per-pixel bounds checks. */
    if(l + size <= frame->width && t + size <= frame->height) {
        uint32_t* dst = frame->data + t * frame->width + l;
        for(uint32_t y = 0; y < size; y++) {
            uint8_t row = glyph[y];
            if(row == 0) { dst += frame->width; continue; }  /* skip empty rows */
            for(uint32_t x = 0; x < size; x++) {
                if(row & 0x80) dst[x] = colour;
                row <<= 1;
            }
            dst += frame->width;
        }
    }
    /* Slow path: clipped — bounds-checked per pixel. */
    else {
        for(uint32_t y = 0; y < size; y++) {
            uint8_t row = glyph[y];
            if(row == 0) continue;
            for(uint32_t x = 0; x < size; x++) {
                if(row & 0x80) ff_frame_set(frame, x + l, y + t, colour);
                row <<= 1;
            }
        }
    }
}

/** @brief Draw a text label using the embedded bitmap font.
 *  @param frame   Target frame.
 *  @param font    Font bitmap (size bytes per glyph).
 *  @param size    Glyph width/height in pixels (character advance).
 *  @param text    Null-terminated string (ASCII 32–127).
 *  @param l       Left pixel position.
 *  @param t       Top pixel position.
 *  @param _w      Unused (reserved for future clipping).
 *  @param _h      Unused (reserved for future clipping).
 *  @param colour  32-bit RGBA colour.
 */
static inline void ffDrawTextLabel(Frame* restrict frame, const uint8_t* restrict font, uint32_t size, const char* restrict text, uint32_t l, uint32_t t, uint32_t, uint32_t, const uint32_t colour) {
    uint32_t n = strlen(text);
    ff_frame_mark(frame, (int32_t)l, (int32_t)t, (int32_t)(l + n * size - 1), (int32_t)(t + size - 1));
    for(uint32_t i = 0; i < n; i++) {
        ff_draw_glyph(frame, font, size, text[i] - 32, l + i * size, t, colour);
    }
}

/** Sprite Operations *********************************************************************************************************/

/** @brief Allocate a sprite with @c nframes frames of size @c w × @c h.
 *  @param sprite   Sprite to initialise.
 *  @param w        Frame width in pixels.
 *  @param h        Frame height in pixels.
 *  @param nframes  Number of animation frames.
 */
static inline void ff_sprite_init(Sprite* restrict sprite, uint32_t w, uint32_t h, uint32_t nframes) {
    Frame temp;
    ff_frame_init(&temp, w, h);

    sprite->data = (Frame*)malloc(nframes * sizeof(temp));
    sprite->width = w;
    sprite->height = h;
    sprite->nframes = nframes;

    ff_frame_flush(&temp);
    
    for(uint32_t i = 0; i < sprite->nframes; i++) {
        ff_frame_init(&sprite->data[i], w, h);
    }
}

/** @brief Free all frame buffers and the sprite's frame array.
 *  @param sprite  Sprite to release.
 */
static inline void ff_sprite_flush(Sprite* restrict sprite) {
    for(uint32_t i = 0; i < sprite->nframes; i++) {
        ff_frame_flush(&sprite->data[i]);
    }
    free(sprite->data);
}

/** @brief Load a horizontal stripe image into sprite frames.
 *  @details The source frame must be @c w × (h * nframes); each frame
 *           is extracted from a consecutive vertical slice.
 *  @param sprite  Target sprite (must be pre-allocated with matching dimensions).
 *  @param frame   Source stripe image.
 */
static inline void ff_sprite_load_stripe(Sprite* restrict sprite, const Frame* restrict frame) {
    for(uint32_t i = 0; i < sprite->nframes; i++) {
        for(uint32_t y = 0; y < sprite->height; y++) {
            for(uint32_t x = 0; x < sprite->width; x++) {
                ff_frame_set(&sprite->data[i], x, y, ff_frame_get(frame, x, y + (i * sprite->height)));
            }
        }
    }
}

/** @brief Draw a bounding-box outline from an @ref ltrb32u.
 *  @param canvas  Target frame.
 *  @param r       Bounding box (left, top, right, bottom).
 *  @param colour  32-bit RGBA colour.
 */
static inline void ff_draw_ltrb_o(Frame* restrict canvas, const ltrb32u* restrict r, const uint32_t colour) {
    ff_frame_mark(canvas, (int32_t)r->l, (int32_t)r->t, (int32_t)r->r, (int32_t)r->b);
    for(uint32_t i = r->l; i <= r->r; ++i) {
        ff_frame_set(canvas, i, r->t, colour);
        ff_frame_set(canvas, i, r->b, colour);
    }
    for(uint32_t i = r->t; i <= r->b; i++) {
        ff_frame_set(canvas, r->l, i, colour);
        ff_frame_set(canvas, r->r, i, colour);
    }
}

/** @brief Draw a filled bounding box from an @ref ltrb32u.
 *  @param canvas  Target frame.
 *  @param r       Bounding box (left, top, right, bottom).
 *  @param colour  32-bit RGBA colour.
 */
static inline void ff_draw_ltrb_f(Frame* restrict canvas, const ltrb32u* restrict r, const uint32_t colour)
{
    ff_frame_mark(canvas, (int32_t)r->l, (int32_t)r->t, (int32_t)r->r, (int32_t)r->b);
    for(uint32_t y = r->t; y <= r->b; ++y) {
        for(uint32_t x = r->l; x <= r->r; ++x) {
            ff_frame_set(canvas, x, y, colour);
        }
    }
}
