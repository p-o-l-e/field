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
 * - Embedded 7×8 bitmap font (@ref gtFont).
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
    uint32_t* data;     ///< Pixel array (width × height entries).
    uint32_t width;     ///< Buffer width in pixels.
    uint32_t height;    ///< Buffer height in pixels.
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

/** @brief Embedded 7×8 bitmap font (ASCII 32–127 + special glyphs). */
extern const uint8_t gtFont[];

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

/** @brief Allocate a frame buffer of the given dimensions.
 *  @param frame  Frame to initialise.
 *  @param x      Width in pixels.
 *  @param y      Height in pixels.
 */
static void     ff_frame_init(Frame* restrict, uint32_t, uint32_t);

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

/** @brief Draw a single glyph from a 7×8 bitmap font.
 *  @param frame  Target frame.
 *  @param font   Font bitmap (7 bytes per glyph).
 *  @param id     Glyph index.
 *  @param l      Left pixel position.
 *  @param t      Top pixel position.
 *  @param colour 32-bit RGBA colour.
 */
static void ff_draw_glyph(Frame* restrict, const uint8_t* restrict, uint32_t, uint32_t, uint32_t, const uint32_t);

/** @brief Draw a text string using the embedded bitmap font.
 *  @param frame  Target frame.
 *  @param font   Font bitmap.
 *  @param text   Null-terminated string (ASCII 32–127).
 *  @param l      Left pixel position.
 *  @param t      Top pixel position.
 *  @param _w     Unused (reserved).
 *  @param _h     Unused (reserved).
 *  @param colour 32-bit RGBA colour.
 */
static void ffDrawTextLabel(Frame* restrict, const uint8_t* restrict, const char* restrict, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);

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

/** @brief Allocate and zero-initialise a frame buffer.
 *  @param frame  Frame to initialise.
 *  @param x      Width in pixels.
 *  @param y      Height in pixels.
 */
static inline void ff_frame_init(Frame* restrict frame, uint32_t x, uint32_t y) {
    frame->width     = x;
    frame->height    = y;
    frame->data      = (uint32_t*)calloc(frame->width * frame->height , sizeof(uint32_t));
    frame->is_dirty = false;
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

/** @brief Draw a single 7×8 glyph from a bitmap font.
 *  @param frame   Target frame.
 *  @param font    Font bitmap (7 bytes per glyph, 1 bit per pixel, LSB = top).
 *  @param id      Glyph index.
 *  @param l       Left pixel position.
 *  @param t       Top pixel position.
 *  @param colour  32-bit RGBA colour.
 */
static inline void ff_draw_glyph(Frame* restrict frame, const uint8_t* restrict font, uint32_t id, uint32_t l, uint32_t t, const uint32_t colour) {
    ff_frame_mark(frame, (int32_t)l, (int32_t)t, (int32_t)(l + 6), (int32_t)(t + 7));
    uint32_t pos = id * 7;
    u_int8_t stencil = 0b1;

    for(uint32_t y = 0; y < 8; y++) {
        for(uint32_t x = 0; x < 7; x++) {
            if(font[pos + x] & stencil) ff_frame_set(frame, x + l, y + t, colour);
        }
        stencil<<=1;
    }
}

/** @brief Draw a text label using the embedded bitmap font.
 *  @param frame   Target frame.
 *  @param font    Font bitmap (7 bytes per glyph).
 *  @param text    Null-terminated string (ASCII 32–127).
 *  @param l       Left pixel position.
 *  @param t       Top pixel position.
 *  @param _w      Unused (reserved for future clipping).
 *  @param _h      Unused (reserved for future clipping).
 *  @param colour  32-bit RGBA colour.
 */
static inline void ffDrawTextLabel(Frame* restrict frame, const uint8_t* restrict font, const char* restrict text, uint32_t l, uint32_t t, uint32_t, uint32_t, const uint32_t colour) {
    uint32_t n = strlen(text);
    ff_frame_mark(frame, (int32_t)l, (int32_t)t, (int32_t)(l + n * 8 - 1), (int32_t)(t + 7));
    for(uint32_t i = 0; i < n; i++) {
        ff_draw_glyph(frame, font, text[i] - 32, l + i * 8, t, colour);
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

/** Font Data *****************************************************************************************************************/

/**
 * @brief Embedded 7×8 bitmap font.
 * @details 7 bytes per glyph, 1 bit per pixel (LSB = top row).
 *          Glyphs 0–95 map to ASCII 32–127. Glyphs 96+ are special symbols
 *          (squares, ramps, progress bars, etc.).
 *          Used by ff_draw_glyph() and ffDrawTextLabel().
 */
const uint8_t gtFont[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0  = space
  0x00,0x00,0x00,0x5F,0x00,0x00,0x00, // 1  = !
  0x00,0x00,0x07,0x00,0x07,0x00,0x00, // 2  = "
  0x14,0x14,0x7f,0x14,0x7f,0x14,0x14, // 3  = #
  0x2E,0x2A,0x2A,0x6B,0x2A,0x2A,0x3A, // 4  = $
  0x43,0x23,0x10,0x08,0x04,0x62,0x61, // 5  = %
  0x00,0x34,0x4A,0x52,0x24,0x40,0x00, // 6  = &
  0x00,0x00,0x00,0x07,0x00,0x00,0x00, // 7  = '
  0x00,0x00,0x1C,0x22,0x41,0x00,0x00, // 8  = (
  0x00,0x00,0x41,0x22,0x1C,0x00,0x00, // 9  = )
  0x00,0x08,0x2A,0x1C,0x2A,0x08,0x00, // 10 = *
  0x00,0x08,0x08,0x3e,0x08,0x08,0x00, // 11 = +
  0x00,0x00,0x00,0x60,0x00,0x00,0x00, // 12 = ,
  0x00,0x08,0x08,0x08,0x08,0x08,0x08, // 13 = -
  0x00,0x00,0x00,0x20,0x00,0x00,0x00, // 14 = .
  0x40,0x20,0x10,0x08,0x04,0x02,0x01, // 15 = /
  0x1c,0x22,0x41,0x41,0x41,0x22,0x1c, // 16 = 0
  0x41,0x41,0x41,0x7f,0x40,0x40,0x40, // 17 = 1
  0x71,0x49,0x49,0x49,0x49,0x49,0x46, // 18 = 2
  0x41,0x41,0x41,0x49,0x49,0x49,0x36, // 19 = 3
  0x18,0x14,0x12,0x11,0x7f,0x10,0x10, // 20 = 4
  0x4f,0x49,0x49,0x49,0x49,0x49,0x31, // 21 = 5
  0x3e,0x49,0x49,0x49,0x49,0x49,0x30, // 22 = 6
  0x01,0x41,0x21,0x11,0x09,0x05,0x03, // 23 = 7
  0x36,0x49,0x49,0x49,0x49,0x49,0x36, // 24 = 8
  0x06,0x49,0x49,0x49,0x49,0x49,0x3e, // 25 = 9
  0x00,0x00,0x00,0x22,0x00,0x00,0x00, // 26 = :
  0x00,0x00,0x00,0x62,0x00,0x00,0x00, // 27 = ;
  0x00,0x08,0x14,0x22,0x41,0x00,0x00, // 28 = <
  0x00,0x14,0x14,0x14,0x14,0x14,0x14, // 29 = =
  0x00,0x00,0x41,0x22,0x14,0x08,0x00, // 30 = >
  0x03,0x01,0x01,0x59,0x09,0x09,0x0F, // 31 = ?
  0x1C,0x22,0x49,0x55,0x55,0x55,0x5F, // 32 = @
  0x7c,0x12,0x11,0x11,0x11,0x12,0x7c, // 33 = A
  0x7f,0x49,0x49,0x49,0x49,0x49,0x36, // 34 = B
  0x1c,0x22,0x41,0x41,0x41,0x41,0x41, // 35 = C
  0x7f,0x41,0x41,0x41,0x41,0x22,0x1c, // 36 = D
  0x7f,0x49,0x49,0x49,0x41,0x41,0x41, // 37 = E
  0x7f,0x09,0x09,0x09,0x01,0x01,0x01, // 38 = F
  0x1c,0x22,0x41,0x41,0x41,0x41,0x79, // 39 = G
  0x7f,0x08,0x08,0x08,0x08,0x08,0x7f, // 40 = H
  0x41,0x41,0x41,0x7f,0x41,0x41,0x41, // 41 = I
  0x10,0x20,0x40,0x41,0x41,0x21,0x1f, // 42 = J
  0x7f,0x08,0x08,0x08,0x14,0x22,0x41, // 43 = K
  0x7f,0x40,0x40,0x40,0x40,0x40,0x40, // 44 = L
  0x7f,0x02,0x04,0x08,0x04,0x02,0x7f, // 45 = M
  0x7f,0x02,0x04,0x08,0x10,0x20,0x7f, // 46 = N
  0x1c,0x22,0x41,0x41,0x41,0x22,0x1c, // 47 = O
  0x7f,0x09,0x09,0x09,0x09,0x09,0x06, // 48 = P
  0x1c,0x22,0x41,0x41,0x51,0x22,0x5c, // 49 = Q
  0x7f,0x09,0x09,0x09,0x19,0x29,0x46, // 50 = R
  0x46,0x49,0x49,0x49,0x49,0x49,0x31, // 51 = S
  0x01,0x01,0x01,0x7f,0x01,0x01,0x01, // 52 = T
  0x1f,0x20,0x40,0x40,0x40,0x20,0x1f, // 53 = U
  0x0f,0x10,0x20,0x40,0x20,0x10,0x0f, // 54 = V
  0x7f,0x20,0x10,0x08,0x10,0x20,0x7f, // 55 = W
  0x41,0x22,0x14,0x08,0x14,0x22,0x41, // 56 = X
  0x01,0x02,0x04,0x78,0x04,0x02,0x01, // 57 = Y
  0x41,0x61,0x51,0x49,0x45,0x43,0x41, // 58 = Z
  0x00,0x00,0x7f,0x41,0x41,0x00,0x00, // 59 = [
  0x01,0x02,0x04,0x08,0x10,0x20,0x40, // 60 = backslash
  0x00,0x00,0x41,0x41,0x7f,0x00,0x00, // 61 = ]
  0x08,0x0c,0x06,0x03,0x06,0x0c,0x08, // 62 = ^
  0x80,0x80,0x80,0x80,0x80,0x80,0x80, // 63 = _
  0x00,0x00,0x03,0x07,0x04,0x00,0x00, // 64 = '\n
  0x00,0x64,0x54,0x54,0x54,0x78,0x00, // 65 = a
  0x00,0x7f,0x44,0x44,0x44,0x38,0x00, // 66 = b
  0x00,0x38,0x44,0x44,0x44,0x44,0x00, // 67 = c
  0x00,0x30,0x48,0x48,0x48,0x7f,0x00, // 68 = d
  0x00,0x38,0x54,0x54,0x54,0x18,0x00, // 69 = e
  0x00,0x08,0x7E,0x09,0x01,0x02,0x00, // 70 = f
  0x00,0x18,0xA4,0xA4,0xA4,0x78,0x00, // 71 = g
  0x00,0x7F,0x04,0x04,0x04,0x78,0x00, // 72 = h
  0x00,0x00,0x44,0x7D,0x44,0x00,0x00, // 73 = i
  0x00,0x20,0x40,0x40,0x44,0x3D,0x00, // 74 = j
  0x00,0x7F,0x10,0x10,0x28,0x44,0x00, // 75 = k
  0x02,0x02,0x3E,0x40,0x40,0x40,0x00, // 76 = l
  0x00,0x7C,0x08,0x10,0x08,0x7C,0x00, // 77 = m
  0x00,0x7C,0x08,0x04,0x04,0x78,0x00, // 78 = n
  0x00,0x38,0x44,0x44,0x44,0x38,0x00, // 79 = o
  0x00,0xFC,0x24,0x24,0x24,0x18,0x00, // 80 = p
  0x00,0x18,0x24,0x24,0x24,0xF8,0x00, // 81 = q
  0x00,0x7C,0x08,0x04,0x04,0x08,0x00, // 82 = r
  0x00,0x58,0x54,0x54,0x54,0x34,0x00, // 83 = s
  0x00,0x04,0x3E,0x44,0x44,0x20,0x00, // 84 = t
  0x00,0x3C,0x40,0x40,0x40,0x7C,0x00, // 85 = u
  0x00,0x1C,0x20,0x40,0x20,0x1C,0x00, // 86 = v
  0x00,0x3C,0x40,0x30,0x40,0x3C,0x00, // 87 = w
  0x00,0x44,0x28,0x10,0x28,0x44,0x00, // 88 = x
  0x00,0x0C,0x90,0x90,0x90,0x7C,0x00, // 89 = y
  0x00,0x44,0x64,0x54,0x4C,0x44,0x00, // 90 = z
  0x00,0x08,0x08,0x36,0x41,0x41,0x00, // 91 = {
  0x00,0x00,0x00,0xFF,0x00,0x00,0x00, // 92 = |
  0x00,0x41,0x41,0x36,0x08,0x08,0x00, // 93 = }
  0x02,0x03,0x01,0x03,0x02,0x03,0x01, // 94 = ~
  0x70,0x78,0x4c,0x46,0x4c,0x78,0x70, // 95 = DEL : ASCII = 127

  0xFF,0x01,0x01,0x01,0x01,0x01,0xFF, // 96 = SQUARE 1
  0x80,0x80,0x80,0x80,0x80,0x80,0xFF, // 97 = SQUARE 2
  0x40,0x20,0x10,0x08,0x04,0x02,0xFF, // 98 = RAMP
  0xFF,0x02,0x04,0x08,0x10,0x20,0x40, // 99 = SAW

  0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 100 = PROGRESSh 0 0x84
  0x7E,0x00,0x00,0x00,0x00,0x00,0x00, // 101 = PROGRESSh 1 0x85
  0x7E,0x00,0x7E,0x00,0x00,0x00,0x00, // 102 = PROGRESSh 2 0x86
  0x7E,0x00,0x7E,0x00,0x7E,0x00,0x00, // 103 = PROGRESSh 3 0x87
  0x7E,0x00,0x7E,0x00,0x7E,0x00,0x7E, // 104 = PROGRESSh 4 0x88

  0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 105 = PROGRESSv 0 0x89
  0x80,0x80,0x80,0x80,0x80,0x80,0x80, // 106 = PROGRESSv 1 0x8A
  0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0, // 107 = PROGRESSv 2 0x8B
  0xA8,0xA8,0xA8,0xA8,0xA8,0xA8,0xA8, // 108 = PROGRESSv 3 0x8C
  0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA, // 109 = PROGRESSv 4 0x8D
  0x10,0x10,0x10,0x10,0x10,0x10,0x10, // 105 H Line    0x8E
  0x10,0x10,0x10,0x1F,0x10,0x10,0x10, // 106 Inv T     0x8F
  0x10,0x10,0x10,0xFF,0x10,0x10,0x10, // 107 Cross     0x90
  0x00,0x3E,0x3E,0x3E,0x3E,0x3E,0x00, // 108 QuadMark  0x91

  0x7F,0x41,0xDD,0x1C,0xDD,0x41,0x7F, // 109 SymbolF   0x92
  0x80,0x80,0xFF,0x00,0xFF,0x80,0x80, // 110 SymbolT   0x93
  0x01,0x01,0xFF,0x00,0xFF,0x01,0x01, // 111 SymbolT   0x94

  0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00

  };
