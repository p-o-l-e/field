/**
 * @file field.h
 * @brief Immediate-mode UI field framework — canvas, entities, widgets, and rendering.
 *
 * @details This header implements the core of a graphical modular-synthesizer-style
 * patcher canvas. It provides:
 * - A layered @ref Field with hit-test stencil buffers for mouse interaction.
 * - @ref Node containers that own arrays of @ref Entity widgets.
 * - Widget types: sliders, encoders, rotaries, checkboxes, buttons, sockets,
 *   textboxes, CRT displays, and nested nodes.
 * - CPU-side @ref Frame pixel buffers and @ref Sprite multi-frame images.
 * - Software draw primitives (rectangles, lines, circles, glyphs).
 * - Dispatch tables for per-type init/set/draw/drag/scroll/enter/leave/release.
 *
 * The design is header-only: all functions are defined inline in this file.
 * Rendering targets are @ref Frame buffers (32-bit RGBA pixel arrays); the
 * caller is responsible for blitting the composited layers to the screen.
 *
 * @note The frame loop and graph evaluation are expected to be allocation-free;
 *       this file allocates only during entity/field/node creation and teardown.
 */

#pragma  once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdatomic.h>

/** @brief Enables the debug overlay layer (extra @ref FieldLayer::DO frame). */
#define DEBUG_OVERLAY
#ifdef DEBUG_OVERLAY
    /** @brief Atomic flag set by the debug overlay to request a forced repaint. */
    atomic_bool force_repaint = false;
#endif

/** @brief Atomic flag to trigger a stream update (e.g. CRT trace redraw). */
atomic_bool trigger_stream_update = false;
/** @brief Number of segments used when drawing Bezier spline cords between sockets. */
constexpr uint_fast32_t SPLINE_SEGMENTS = 32;
/** @brief Fixed size of the text buffer inside a @ref Textbox extension. */
constexpr uint_fast32_t TEXTBOX_SIZE = 32;

/** @brief Root field container — owns layers, nodes, and interaction state. */
typedef struct Field Field;
/** @brief A single widget/entity on the canvas (slider, socket, button, …). */
typedef struct Entity Entity;
/** @brief Extension data for checkbox entities. */
typedef struct Checkbox Checkbox;
/** @brief Extension data for slider entities. */
typedef struct Slider Slider;
/** @brief Extension data for encoder entities. */
typedef struct Encoder Encoder;
/** @brief Extension data for momentary button entities. */
typedef struct Momentary Momentary;
/** @brief Extension data for rotary / infinite-slider entities. */
typedef struct Rotary Rotary;
/** @brief Extension data for socket (patch-cable) entities. */
typedef struct Socket Socket;
/** @brief Extension data for CRT / oscillograph display entities. */
typedef struct Oscillograph Oscillograph;
/** @brief Extension data for textbox (label) entities. */
typedef struct Textbox Textbox;
/** @brief Extension data for nested-node entities. */
typedef struct EntityNode EntityNode;
/** @brief Extension data for sprite-button entities. */
typedef struct SpriteButton SpriteButton;
/** @brief A container node holding an array of entities. */
typedef struct Node Node;
/** @brief Blueprint used to instantiate an @ref Entity via ffCreateEntity(). */
typedef struct SectorDescriptor SectorDescriptor;

/**
 * @brief Widget type — indexes into the dispatch tables.
 * @note ST_LIMIT must remain the last entry; it is the table size.
 */
typedef enum {
    ST_SLIDER,          ///< Linear slider (horizontal or vertical).
    ST_ENCODER,         ///< Endless rotary encoder with wrap-around.
    ST_ROTARY,          ///< Rotary knob with sprite frames.
    ST_SPRITE_INF_SLIDER, ///< Infinite slider rendered via sprite frames.
    ST_SOCKET,          ///< Patch-cable socket (input or output).
    ST_CHECKBOX,        ///< Toggle checkbox.
    ST_MOMENTARY,       ///< Momentary push button.
    ST_SPRITE_CHECKBOX, ///< Checkbox rendered with sprite frames.
    ST_SPRITE_BUTTON,   ///< Button rendered with sprite frames.
    ST_CANVAS,          ///< Background canvas (grid drawing).
    ST_TEXTBOX,         ///< Read-only text label.
    ST_CRT,             ///< CRT / oscillograph display.
    ST_NODE,            ///< Nested node (sub-patch container).

    ST_LIMIT            ///< Sentinel — number of sector types.

} SectorType;

/**
 * @brief Visual subtype for sliders and encoders.
 */
typedef enum {
    SS_A,       ///< Slider style A — filled bar from start to value.
    SS_B,       ///< Slider style B — filled bar from value to end.
    SS_C,       ///< Slider style C — bipolar, filled from zero to value.

    SS_LIMIT    ///< Sentinel — number of subtypes.

} SubType;

/**
 * @brief Core role of an entity in the data-flow graph.
 */
typedef enum: uint8_t {
    F_CT_STATIC,    ///< Purely visual; no data flow.
    F_CT_CONTROL,   ///< User-adjustable control (slider, knob, …).
    F_CT_INPUT,     ///< Data input (receives a value via a socket).
    F_CT_OUTPUT     ///< Data output (provides a value via a socket).
} CoreType;

/**
 * @brief Callback event types — indexes into Entity::callback and Entity::target.
 */
typedef enum {
    F_CE_PRESS,     ///< Fired on mouse-button press.
    F_CE_RELEASE,   ///< Fired on mouse-button release.
    F_CE_VALUE,     ///< Fired when the entity value changes.
    F_CE_REPAINT,   ///< Fired after the entity is redrawn.

    F_CE_LIMIT      ///< Sentinel — number of callback events.

} CallbackEvent;

/**
 * @brief Index positions for hit-test results stored in Field::current / Field::prior.
 */
typedef enum {
    IP_NODE,        ///< Index into the node array.
    IP_ENTITY,      ///< Index into the entity array of that node.

    IP_LIMIT        ///< Sentinel — number of index positions.

} IndexPosition;

/**
 * @brief Mouse button and keyboard modifier bitmask values.
 * @note Combine with bitwise OR to test for multiple modifiers.
 */
typedef enum: uint32_t {
    LMB     = 1 << 0,   ///< Left mouse button.
    MMB     = 1 << 1,   ///< Middle mouse button.
    RMB     = 1 << 2,   ///< Right mouse button.
    LCTRL   = 1 << 3,   ///< Left Ctrl key.
    RCTRL   = 1 << 4,   ///< Right Ctrl key.
    LSHIFT  = 1 << 5,   ///< Left Shift key.
    RSHIFT  = 1 << 6,   ///< Right Shift key.

} ControlModifier;

/**
 * @brief Value precision modes — coarse (integer) and fine (fractional).
 * @note Entity::value and Slider::step / Encoder::step are indexed by this enum.
 */
typedef enum {
    CP_COARSE,      ///< Coarse (integer) component of the value.
    CP_FINE,        ///< Fine (fractional) component of the value.

    CP_LIMIT        ///< Sentinel — number of precision modes.

} ControlPrecision;

/**
 * @brief Saved cursor positions in Field::memory — used during drag operations.
 */
typedef enum {
    SP_CURSOR_PRESS,    ///< Cursor position at mouse-button press.
    SP_CURSOR_PRIOR,    ///< Cursor position at the previous frame (drag delta source).
    SP_LT_PRESS,        ///< Entity bounds left/top at press time (for snap-back).

    SP_LIMIT            ///< Sentinel — number of saved positions.

} SavedPosition;

/**
 * @brief Render layer indices — each layer is a full-field @ref Frame.
 * @note Layer order (bottom to top): BG, NG, TL, FG, SC, SN, [DO], ST.
 *       CC is the total count (sentinel).
 */
typedef enum: uint32_t {
    BG,                         ///< Background grid.
    NG,                         ///< Node backgrounds.
    TL,                         ///< Text layer.
    FG,                         ///< Foreground (widget drawing).
    SC,                         ///< Stencil — entity hit-test IDs (controls).
    SN,                         ///< Stencil — node hit-test IDs.
    ST,                         ///< Staging layer (drag preview).
    #ifdef DEBUG_OVERLAY
        DO,                     ///< Debug overlay layer.
    #endif
    CC,                         ///< Sentinel — total layer count.

} FieldLayer;

/**
 * @brief Per-entity behaviour flags — combine with bitwise OR.
 */
typedef enum: uint32_t {
    VERTICAL    = 1 << 0,   ///< Widget is oriented vertically.
    MOVEABLE    = 1 << 1,   ///< Entity can be dragged to a new position.
    INTERCON    = 1 << 2,   ///< Entity participates in socket connections.
    TRANSPARENT = 1 << 3,   ///< Entity is transparent to hit-testing (no stencil fill).
    RADIO       = 1 << 4,   ///< Entity is part of a radio group (checkbox).

} SectorFlags;

/**
 * @brief Per-field behaviour flags.
 */
typedef enum {
    ROOT        = 1 << 0,   ///< This field is the root field.

} FieldFlags;

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

/**
 * @brief Blueprint describing an entity to be created via ffCreateEntity().
 * @details Fill this struct and pass it to ffCreateEntity() to instantiate
 *          a widget inside a @ref Node. Most fields map directly to @ref Entity
 *          or extension-struct fields.
 */
struct SectorDescriptor {
    uint32_t    id;                 ///< Unique entity identifier (uid).
    uint32_t    node_id;            ///< Target node identifier.
    ltwh32u     bounds;             ///< Position and size within the node.
    CoreType    core_type;          ///< Data-flow role (static / control / input / output).
    SectorType  type;               ///< Widget type (determines extension struct).
    SubType     subtype;            ///< Visual subtype (for sliders / encoders).
    float       default_value;      ///< Initial value.
    float       range[2];           ///< [0] = minimum, [1] = maximum.
    float       step[CP_LIMIT];     ///< Step size per precision mode (coarse / fine).
    uint32_t    radio_id;           ///< Radio group ID (only when RADIO flag is set).
    char*       label;              ///< Label text (used by ST_TEXTBOX).
    uint32_t    flags;              ///< Behaviour flags (@ref SectorFlags).
    uint32_t    callback_target_id; ///< UID of the entity to receive callbacks.
    uint32_t    input_x;            ///< UID of the X-input socket (ST_CRT only).
    uint32_t    input_y;            ///< UID of the Y-input socket (ST_CRT only).
    CallbackEvent callback_event;   ///< Which event triggers the callback.
};

/**
 * @brief A single widget / entity on the canvas.
 * @details Entities are stored in contiguous arrays inside @ref Node.
 *          Type-specific data lives in the @c extension pointer (see the
 *          extension structs below). The @c type field indexes into the
 *          dispatch tables (init_entity, set_entity, …).
 */
struct Entity {
    uint32_t    index;          ///< Position in parent node's entity array.
    uint32_t    uid;            ///< Unique identifier.
    ltrb32u     bounds;         ///< Screen-space bounding box.
    uint32_t    width;          ///< Cached bounds width.
    uint32_t    height;         ///< Cached bounds height.
    CoreType    core_type;      ///< F_CT_STATIC / F_CT_CONTROL / F_CT_INPUT / F_CT_OUTPUT.
    SectorType  type;           ///< Widget type (dispatch table index).
    float       value[CP_LIMIT]; ///< Current value (coarse integer + fine fractional).
    bool        visible;        ///< Render visibility.
    bool        hovered;        ///< Cursor hover state.
    bool        repaint;        ///< Dirty flag for redraw.
    uint32_t    flags;          ///< VERTICAL, MOVEABLE, INTERCON, TRANSPARENT, RADIO.
    void*       extension;      ///< Type-specific extension struct.
    void        (*callback[F_CE_LIMIT])(Entity*, Entity*); ///< Event callbacks.
    Entity*     target[F_CE_LIMIT]; ///< Callback target entities.
    Node*       parent;         ///< Owning node.
};

/** @name Extension structs
 * @brief Type-specific data lives here, not in Entity.
 * @{ */

/**
 * @brief Extension data for checkbox entities (ST_CHECKBOX, ST_SPRITE_CHECKBOX).
 */
struct Checkbox {
    uint32_t    radio_id;       ///< Radio group identifier (RADIO flag).
    Sprite*     sprite;         ///< Sprite frames (ST_SPRITE_CHECKBOX only).
};

/**
 * @brief Extension data for slider entities (ST_SLIDER).
 */
struct Slider {
    SubType     type;           ///< Visual subtype (SS_A / SS_B / SS_C).
    float       default_value;  ///< Default value at reset.
    float       step[CP_LIMIT]; ///< Step size per precision mode.
    float       memory[CP_LIMIT]; ///< Value snapshot at drag start.
    float       range[2];       ///< Min / max value.
    bool        inverse;        ///< Inverted drag direction.
};

/**
 * @brief Extension data for encoder entities (ST_ENCODER).
 * @details Unlike a slider, an encoder wraps around at range edges by
 *          toggling the @c inverse flag.
 */
struct Encoder {
    SubType     type;           ///< Visual subtype.
    float       default_value;  ///< Default value at reset.
    float       step[CP_LIMIT]; ///< Step size per precision mode.
    float       memory[CP_LIMIT]; ///< Value snapshot at drag start.
    float       range[2];       ///< Min / max value.
    bool        inverse;        ///< Inverted drag direction (toggles at range edges).
};

/**
 * @brief Extension data for rotary knob and infinite-slider entities
 *        (ST_ROTARY, ST_SPRITE_INF_SLIDER).
 */
struct Rotary {
    float       default_value;  ///< Default value at reset.
    float       step[CP_LIMIT]; ///< Step size per precision mode.
    float       range[2];       ///< Min / max value.
    Sprite*     sprite;         ///< Sprite frames for rendering.
};

/**
 * @brief Extension data for socket entities (ST_SOCKET).
 * @details Sockets form patch-cable connections between modules.
 *          An input socket receives a pointer to the connected output's value.
 */
struct Socket {
    float*      input;          ///< Input value pointer (from connected output socket).
    float*      output;         ///< Output value pointer (owned by core module).
    float*      cord;           ///< Bezier spline control points (SPLINE_SEGMENTS * 2 floats).
    Entity*     connection;     ///< Connected socket entity.
    bool        connected;      ///< Has active connection.
    bool        has_data;       ///< Cord spline data is valid.
};

/**
 * @brief Extension data for CRT / oscillograph display entities (ST_CRT).
 * @details Plots an X-Y trace on a graticule using a ring buffer.
 */
struct Oscillograph {
    float**     x;              ///< X input source pointer (pointer to Socket::input).
    float**     y;              ///< Y input source pointer (pointer to Socket::input).
    void*       data;           ///< Internal float buffer for streaming.
    Frame*      frame;          ///< CRT frame buffer (graticule + trace).
    uint32_t    r_head;         ///< Ring buffer read head.
    uint32_t    w_head;         ///< Ring buffer write head.
    uint32_t    fps;            ///< Target frames per second for streaming redraw.
};

/**
 * @brief Extension data for textbox / label entities (ST_TEXTBOX).
 */
struct Textbox {
    char        text[TEXTBOX_SIZE]; ///< Display text buffer (fixed size).
};

/**
 * @brief Extension data for nested-node entities (ST_NODE).
 */
struct EntityNode {
    Frame*      frame;          ///< Node frame buffer (render target for child entities).
    bool        staging;        ///< Drag staging state (rendered to staging layer).
};

/**
 * @brief Extension data for sprite-button entities (ST_SPRITE_BUTTON).
 */
struct SpriteButton {
    Sprite*     sprite;         ///< Sprite frames (on/off states).
};

/**
 * @brief Extension data for momentary button entities (ST_MOMENTARY).
 * @note Currently empty — reserved for future use.
 */
struct Momentary {
};

/** @} */

/**
 * @brief A container node holding a contiguous array of @ref Entity objects.
 * @details Nodes are the patcher's modules. Each node has its own @ref Frame
 *          buffer that child entities render into. Node 0 is the root canvas.
 */
struct Node {
    uint32_t    index;              ///< Position in the parent field's node array.
    uint32_t    uid;                ///< Unique node identifier.
    uint32_t    width;              ///< Node width in pixels.
    uint32_t    height;             ///< Node height in pixels.
    uint32_t    entities;           ///< Entity count in this node.
    uint32_t    capacity;           ///< Allocated capacity of the @c at array.
    Entity*     at;                 ///< Entity array (contiguous, index-based).
    Field*      parent;             ///< Owning field.
    Frame       buffer;             ///< Off-screen render target for child entities.
    bool        visible;            ///< Whether the node is visible on the canvas.
};

/**
 * @brief The root field — owns all layers, nodes, and global interaction state.
 * @details A field is a rectangular canvas area. It maintains multiple
 *          @ref Frame layers for compositing, a stencil-based hit-test
 *          system, and the current drag / connection state.
 */
struct Field {
    ltrb32u     bounds;             ///< Field bounding box in screen coordinates.
    uint32_t    width;              ///< Field width in pixels.
    uint32_t    height;             ///< Field height in pixels.
    point32s    memory[SP_LIMIT];   ///< Saved cursor positions (see @ref SavedPosition).
    Frame*      layer[CC];          ///< Render layers (see @ref FieldLayer).
    Node*       node;               ///< Node array (node 0 is the root canvas).
    Entity*     pressed;            ///< Currently pressed entity (drag source).
    Entity*     stream[3];          ///< Streaming entities (e.g. CRT displays).
    uint32_t    current[IP_LIMIT];  ///< Current hit-test result.
    bool        repaint;            ///< Global repaint flag.
    bool        drag;               ///< Sector (node) drag in progress.
    bool        move;               ///< Window move in progress.
    bool        staging;            ///< Staging layer is active.
    bool        connecting;         ///< Socket connection drag in progress.
    uint32_t    nodes;              ///< Number of nodes in the @c node array.
    uint32_t    capacity;           ///< Allocated capacity of the @c node array.
    uint32_t    flags;              ///< Field-level flags (@ref FieldFlags).
    uint32_t    step;               ///< Grid snap step in pixels.
};

/** @brief Embedded 7×8 bitmap font (ASCII 32–127 + special glyphs). */
extern const uint8_t gtFont[];

/** @name Function prototypes
 * @{ */

/** @brief Extract a single byte from a 32-bit value.
 *  @param value  The 32-bit source value.
 *  @param byte   Byte index (0 = LSB, 3 = MSB).
 *  @return The extracted 8-bit value.
 */
uint8_t     ff_extract_byte(uint32_t, uint_fast8_t);

/** @brief Convert normalised UV coordinates to screen pixel coordinates.
 *  @param x  Normalised X (0.0–1.0).
 *  @param y  Normalised Y (0.0–1.0, top-down).
 *  @param w  Target width in pixels.
 *  @param h  Target height in pixels.
 *  @return Screen-space pixel coordinates.
 */
point32u    ff_uv_to_screen(float, float, uint32_t, uint32_t);

/** @brief Convert screen pixel coordinates to normalised UV coordinates.
 *  @param x  Screen X pixel.
 *  @param y  Screen Y pixel (top-down).
 *  @param w  Source width in pixels.
 *  @param h  Source height in pixels.
 *  @return Normalised UV coordinates (Y flipped: 0 at bottom).
 */
point       ff_screen_to_uv(uint32_t, uint32_t, uint32_t, uint32_t);

/** @brief Convert HSVA (0–255 each) to a packed 32-bit RGBA value.
 *  @param h  Hue (0–255).
 *  @param s  Saturation (0–255).
 *  @param v  Value / brightness (0–255).
 *  @param a  Alpha (0–255).
 *  @return Packed 0xRRGGBBAA colour.
 */
static uint32_t ff_hsva_to_rgba(uint8_t, uint8_t, uint8_t, uint8_t);

/** @brief Generate a deterministic colour from an index (golden-ratio hue spread).
 *  @param index  Unique index (e.g. entity index).
 *  @param alpha  Alpha value (0–255).
 *  @return Packed 0xRRGGBBAA colour.
 */
uint32_t    ff_index_to_hsv(uint32_t, uint8_t);

/** @brief Set a single pixel in a frame (bounds-checked).
 *  @param frame  Target frame.
 *  @param x      Pixel X.
 *  @param y      Pixel Y.
 *  @param value  32-bit RGBA value.
 */
static void     ff_frame_set(Frame*, uint32_t, uint32_t, uint32_t);

/** @brief Get a single pixel from a frame (bounds-checked, returns pixel 0 on OOB).
 *  @param frame  Source frame.
 *  @param x      Pixel X.
 *  @param y      Pixel Y.
 *  @return 32-bit RGBA value.
 */
static uint32_t ff_frame_get(Frame*, uint32_t, uint32_t);

/** @brief Clear the entire frame to zero (transparent black).
 *  @param frame  Target frame.
 */
static void     ff_frame_clr(Frame*);

/** @brief Fill the entire frame with a single colour.
 *  @param frame  Target frame.
 *  @param value  32-bit RGBA fill value.
 */
static void     ff_frame_fill(Frame*, uint32_t);

/** @brief Allocate a frame buffer of the given dimensions.
 *  @param frame  Frame to initialise.
 *  @param x      Width in pixels.
 *  @param y      Height in pixels.
 */
static void     ff_frame_init(Frame*, uint32_t, uint32_t);

/** @brief Free a frame's pixel buffer.
 *  @param frame  Frame to release.
 */
static void     ff_frame_flush(Frame*);

/** @brief Copy all pixels from source to target (dimensions must match).
 *  @param target  Destination frame.
 *  @param source  Source frame.
 */
static void     ff_frame_copy(Frame*, Frame*);

/** @brief Copy source to target, recoloring each non-zero pixel via ff_index_to_hsv.
 *  @param target  Destination frame.
 *  @param source  Source frame.
 *  @param alpha   Alpha value for the recolored output.
 */
static void     ff_frame_copy_with_alpha(Frame*, Frame*, uint8_t);

/** @brief Copy source to target at a pixel offset, skipping zero (transparent) pixels.
 *  @param target  Destination frame.
 *  @param source  Source frame.
 *  @param xo      X offset in target.
 *  @param yo      Y offset in target.
 */
static void     ff_frame_copy_at(Frame*, Frame*, uint32_t, uint32_t);

/** @brief Union a modified region into a frame's dirty rectangle (clamped to bounds).
 *  @param frame  Target frame.
 *  @param l      Left edge (inclusive; negative values are clamped to 0).
 *  @param t      Top edge (inclusive; negative values are clamped to 0).
 *  @param r      Right edge (inclusive).
 *  @param b      Bottom edge (inclusive).
 */
static void     ff_frame_mark(Frame*, int32_t, int32_t, int32_t, int32_t);

/** @brief Reset a frame's dirty rectangle (call after the region was uploaded).
 *  @param frame  Target frame.
 */
static void     ff_frame_dirty_reset(Frame*);

/** @brief Allocate a sprite with @c nframes frames of size @c w × @c h.
 *  @param sprite   Sprite to initialise.
 *  @param w        Frame width in pixels.
 *  @param h        Frame height in pixels.
 *  @param nframes  Number of frames.
 */
static void ff_sprite_init(Sprite*, uint32_t, uint32_t, uint32_t);

/** @brief Free all frames and the sprite's frame array.
 *  @param sprite  Sprite to release.
 */
static void ff_sprite_flush(Sprite*);

/** @brief Load a horizontal stripe image into sprite frames.
 *  @details The source frame must be @c w × (h * nframes) pixels;
 *           each frame is extracted from a vertical slice.
 *  @param sprite  Target sprite.
 *  @param frame   Source stripe image.
 */
static void ff_sprite_load_stripe(Sprite*, Frame*);

/** @brief Draw a single glyph from a 7×8 bitmap font.
 *  @param frame  Target frame.
 *  @param font   Font bitmap (7 bytes per glyph).
 *  @param id     Glyph index.
 *  @param l      Left pixel position.
 *  @param t      Top pixel position.
 *  @param colour 32-bit RGBA colour.
 */
static void ff_draw_glyph(Frame*, const uint8_t*, uint32_t, uint32_t, uint32_t, const uint32_t);

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
static void ffDrawTextLabel(Frame*, const uint8_t*, const char*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);

/** @brief Draw a rectangle outline (1-pixel border).
 *  @param frame  Target frame.
 *  @param l      Left edge.
 *  @param t      Top edge.
 *  @param r      Right edge.
 *  @param b      Bottom edge.
 *  @param colour 32-bit RGBA colour.
 */
static void ff_draw_rect_o(Frame*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);

/** @brief Draw a filled rectangle.
 *  @param frame  Target frame.
 *  @param l      Left edge.
 *  @param t      Top edge.
 *  @param r      Right edge.
 *  @param b      Bottom edge.
 *  @param colour 32-bit RGBA colour.
 */
static void ff_draw_rect_f(Frame*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);

/** @brief Draw a vertical line.
 *  @param frame  Target frame.
 *  @param x      X column.
 *  @param yo     Start Y (inclusive).
 *  @param ye     End Y (inclusive).
 *  @param value  32-bit RGBA colour.
 */
static void ff_draw_line_v(Frame*, uint32_t, uint32_t, uint32_t, const uint32_t);

/** @brief Draw a horizontal line.
 *  @param frame  Target frame.
 *  @param y      Y row.
 *  @param xo     Start X (inclusive).
 *  @param xe     End X (inclusive).
 *  @param value  32-bit RGBA colour.
 */
static void ff_draw_line_h(Frame*, uint32_t, uint32_t, uint32_t, const uint32_t);

/** @brief Draw a filled circle (midpoint algorithm).
 *  @param frame  Target frame.
 *  @param xc     Centre X.
 *  @param yc     Centre Y.
 *  @param r      Radius in pixels.
 *  @param value  32-bit RGBA colour.
 */
static void ff_draw_circle_f(Frame*, uint32_t, uint32_t, uint32_t, uint32_t);

/** @brief Draw a circle outline (midpoint algorithm).
 *  @param frame  Target frame.
 *  @param xc     Centre X.
 *  @param yc     Centre Y.
 *  @param r      Radius in pixels.
 *  @param value  32-bit RGBA colour.
 */
static void ff_draw_circle_o(Frame*, uint32_t, uint32_t, uint32_t, uint32_t);

/** @brief Draw a bounding-box outline from an @ref ltrb32u.
 *  @param canvas  Target frame.
 *  @param r       Bounding box.
 *  @param colour  32-bit RGBA colour.
 */
static void ff_draw_ltrb_o(Frame*, ltrb32u*, const uint32_t);

/** @brief Draw a filled bounding box from an @ref ltrb32u.
 *  @param canvas  Target frame.
 *  @param r       Bounding box.
 *  @param colour  32-bit RGBA colour.
 */
static void ff_draw_ltrb_f(Frame*, ltrb32u*, const uint32_t);

/** @brief Create an entity inside a node from a descriptor.
 *  @param node        Target node.
 *  @param descriptor  Entity blueprint (see @ref SectorDescriptor).
 *  @return Pointer to the newly created entity, or @c NULL on failure.
 */
Entity* ffCreateEntity(Node* restrict, SectorDescriptor*);

/** @brief Create a new field (canvas).
 *  @param x      Left position in screen coordinates.
 *  @param y      Top position in screen coordinates.
 *  @param w      Width in pixels.
 *  @param h      Height in pixels.
 *  @param size   Initial node capacity.
 *  @param flags  Field flags (@ref FieldFlags).
 *  @return Pointer to the newly created field.
 */
Field*  ffCreateField(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

/** @brief Destroy a field and free all associated resources.
 *  @param field  Field to destroy.
 */
void    ffDestroyField(Field* restrict);

/** @brief Create a new node inside a field.
 *  @param field  Owning field.
 *  @param w      Node width in pixels.
 *  @param h      Node height in pixels.
 *  @param size   Initial entity capacity.
 *  @return Pointer to the newly created node.
 */
static Node* ffCreateNode(Field* restrict, uint32_t, uint32_t, size_t);

/** @brief Place a node on the canvas at the given position (makes it visible).
 *  @param field  Owning field.
 *  @param node   Node to place.
 *  @param x      Left position in screen coordinates.
 *  @param y      Top position in screen coordinates.
 */
static void  ffPlaceNode(Field* restrict, Node* restrict, uint32_t, uint32_t);

/** @brief Find an entity by its unique ID across all nodes.
 *  @param field  Field to search.
 *  @param id     Entity UID to find (0 returns @c NULL).
 *  @return Pointer to the entity, or @c NULL if not found.
 */
Entity* ff_find_entity_by_id(Field* restrict, uint32_t);

/** @brief No-op placeholder (reserved). */
void    ff_empty(void);

/** @brief No-op callback — default link fuse for unused callback slots.
 *  @param a  Ignored.
 *  @param b  Ignored.
 */
void    ff_fuse_link(Entity*, Entity*);

/** @brief Move an entity to new bounds, updating stencil layers.
 *  @param entity  Entity to move.
 *  @param bounds  New bounding box.
 */
void    ff_move_entity(Entity* restrict, ltrb32u* restrict);

/** @brief Erase an entity (removes from stencil; does not free).
 *  @param entity  Entity to erase.
 */
void    ff_erase_entity(Entity* restrict);

/** @brief Register a modular link (callback) from source to target.
 *  @param source  Source entity (fires the callback).
 *  @param target  Target entity (receives the callback).
 *  @param type    Callback event type (@ref CallbackEvent).
 *  @param fn      Callback function.
 */
void    ff_add_mod_link(Entity*, Entity*, CallbackEvent, void (*)(Entity*, Entity*));

/** @brief Set the text content of a textbox entity.
 *  @param entity  Textbox entity.
 *  @param text    Null-terminated text (truncated to TEXTBOX_SIZE-1).
 */
void    ff_set_text_data(Entity*, const char*);

/** @brief Copy a slider's value into a textbox as a formatted string.
 *  @param slider  Source slider entity.
 *  @param tbox    Target textbox entity.
 */
static void ff_value_to_textbox(Entity*, Entity*);

/** @brief Mark the target entity for repaint.
 *  @param _src    Ignored source entity.
 *  @param target  Entity to mark dirty.
 */
static void ff_set_repaint(Entity*, Entity*);

/** @brief Draw all dirty entities across all visible nodes.
 *  @param field  Field to render.
 */
void    ff_draw_scene(Field* restrict);

/** @brief Handle mouse-button press (hit test + dispatch).
 *  @param field   Target field.
 *  @param x       Mouse X.
 *  @param y       Mouse Y.
 *  @param button  Button/modifier bitmask (@ref ControlModifier).
 */
void ff_hit_test_down(Field* restrict, int, int, uint32_t);

/** @brief Handle mouse drag (forwarded to the pressed entity's drag handler).
 *  @param field  Target field.
 *  @param x      Mouse X.
 *  @param y      Mouse Y.
 */
void ff_hit_test_drag(Field* restrict, int, int);

/** @brief Handle mouse-button release.
 *  @param field   Target field.
 *  @param x       Mouse X.
 *  @param y       Mouse Y.
 *  @param button  Button/modifier bitmask (@ref ControlModifier).
 */
void ff_hit_test_up  (Field* restrict, int, int, uint32_t);

/** @brief Handle mouse move (hover enter/leave detection).
 *  @param field  Target field.
 *  @param x      Mouse X.
 *  @param y      Mouse Y.
 */
void ff_hit_test     (Field* restrict, int, int);

/** @brief Disconnect a socket entity from its current connection.
 *  @param entity  Socket entity to disconnect.
 */
static void ff_disconnect(Entity* restrict);

#ifdef DEBUG_OVERLAY
/** @brief Draw the debug overlay with entity diagnostic information.
 *  @param field   Target field.
 *  @param entity  Entity under the cursor (may be @c NULL).
 */
static void ff_draw_debug_overlay(Field*, Entity*);
#endif

/** @name Dispatch tables
 * @brief Function pointer arrays indexed by @ref SectorType.
 * @{ */
extern void (*init_entity[])(Entity* restrict);             ///< Per-type initialisation.
extern void (*set_entity[])(Entity* restrict, int, int);     ///< Per-type press handler.
extern void (*draw_entity[])(Entity* restrict);              ///< Per-type draw handler.
extern void (*drag_entity[])(Entity* restrict, int, int);    ///< Per-type drag handler.
extern void (*scroll_entity[])(Entity* restrict, int, int);  ///< Per-type scroll handler.
extern void (*enter_entity[])(Entity* restrict, int, int);   ///< Per-type hover-enter handler.
extern void (*leave_entity[])(Entity* restrict, int, int);   ///< Per-type hover-leave handler.
extern void (*release_entity[])(Entity* restrict, int, int); ///< Per-type release handler.
/** @} */
/** @} */

/** Color & Utility ***********************************************************************************************************/

/**
 * @brief Extract a single byte from a 32-bit value.
 * @details Data layout: 0x FF FF FF FF, index 3 2 1 0 (MSB→LSB).
 * @param value  The 32-bit source value.
 * @param byte   Byte index (0 = LSB, 3 = MSB).
 * @return The extracted 8-bit value.
 */
uint8_t ff_extract_byte(uint32_t value, uint_fast8_t byte) {
    return (value >> (byte * 8)) & 0xFF;
}

/** @brief Convert normalised UV coordinates to screen pixel coordinates.
 *  @param x  Normalised X (0.0–1.0).
 *  @param y  Normalised Y (0.0–1.0, top-down).
 *  @param w  Target width in pixels.
 *  @param h  Target height in pixels.
 *  @return Screen-space pixel coordinates.
 */
point32u ff_uv_to_screen(float x, float y, uint32_t w, uint32_t h) {
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
point ff_screen_to_uv(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
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
static uint32_t ff_hsva_to_rgba(uint8_t h, uint8_t s, uint8_t v, uint8_t a) {
    uint8_t region = h / 43;
    uint8_t remainder = (h - region * 43) * 6;

    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

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
uint32_t ff_index_to_hsv(uint32_t index, uint8_t alpha) {
    uint8_t hue = (index * 137) & 0xFF;
    uint8_t sat = 204;
    uint8_t val = 230;
    return ff_hsva_to_rgba(hue, sat, val, alpha);
}

/** Frame Operations **********************************************************************************************************/

/** @brief Set a single pixel in a frame (bounds-checked, no-op if out of range).
 *  @param frame  Target frame.
 *  @param x      Pixel X coordinate.
 *  @param y      Pixel Y coordinate.
 *  @param value  32-bit RGBA pixel value.
 */
static inline void ff_frame_set(Frame* frame, uint32_t x, uint32_t y, uint32_t value) {
    if(x < frame->width && y < frame->height)
    frame->data[x + y * frame->width] = value;
}

/** @brief Get a single pixel from a frame (bounds-checked, returns pixel [0,0] if OOB).
 *  @param frame  Source frame.
 *  @param x      Pixel X coordinate.
 *  @param y      Pixel Y coordinate.
 *  @return 32-bit RGBA pixel value.
 */
static inline uint32_t ff_frame_get(Frame* frame, uint32_t x, uint32_t y) {
    if(x < frame->width && y < frame->height) return frame->data[x + y * frame->width];
    return frame->data[0];
}

/** @brief Clear the entire frame buffer to zero (transparent black).
 *  @param frame  Target frame.
 */
static inline void ff_frame_clr(Frame* frame) {
    memset(frame->data, 0, frame->height * frame->width * sizeof(uint32_t));
    ff_frame_mark(frame, 0, 0, (int32_t)frame->width - 1, (int32_t)frame->height - 1);
}

/** @brief Fill the entire frame with a single colour value.
 *  @param frame  Target frame.
 *  @param value  32-bit RGBA fill value.
 */
static inline void ff_frame_fill(Frame* frame, uint32_t value) {
    for(uint32_t i = 0; i < (frame->height * frame->width); ++i) frame->data[i] = value;
    ff_frame_mark(frame, 0, 0, (int32_t)frame->width - 1, (int32_t)frame->height - 1);
}

/** @brief Allocate and zero-initialise a frame buffer.
 *  @param frame  Frame to initialise.
 *  @param x      Width in pixels.
 *  @param y      Height in pixels.
 */
static inline void ff_frame_init(Frame* frame, uint32_t x, uint32_t y) {
    frame->width     = x;
    frame->height    = y;
    frame->data      = (uint32_t*)calloc(frame->width * frame->height , sizeof(uint32_t));
    frame->is_dirty = false;
}

/** @brief Free the frame's pixel buffer (does not free the Frame struct itself).
 *  @param frame  Frame whose data to free.
 */
static inline void ff_frame_flush(Frame* frame) {
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
static inline void ff_frame_mark(Frame* frame, int32_t l, int32_t t, int32_t r, int32_t b) {
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
static inline void ff_frame_dirty_reset(Frame* frame) {
    frame->is_dirty = false;
}

/** @brief Copy all pixels from source to target (dimensions must match).
 *  @param target  Destination frame.
 *  @param source  Source frame.
 */
static inline void ff_frame_copy(Frame* target, Frame* source) {
    for(uint32_t i = 0; i < target->height * target->width; i++) target->data[i] = source->data[i];
    ff_frame_mark(target, 0, 0, (int32_t)target->width - 1, (int32_t)target->height - 1);
}

/** @brief Copy source to target, recoloring each pixel via ff_index_to_hsv.
 *  @param target  Destination frame.
 *  @param source  Source frame (used as stencil index).
 *  @param alpha   Alpha value for the recolored output.
 */
static void ff_frame_copy_with_alpha(Frame* target, Frame* source, uint8_t alpha) {
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
static inline void ff_frame_copy_at(Frame* target, Frame* source, uint32_t xo, uint32_t yo) {
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
static inline void ff_draw_rect_f(Frame* frame, uint32_t l, uint32_t t, uint32_t r, uint32_t b, const uint32_t colour) {
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
static inline void ff_draw_line_v(Frame* frame, uint32_t x, uint32_t yo, uint32_t ye, const uint32_t value) {
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
static inline void ff_draw_line_h(Frame* frame, uint32_t y, uint32_t xo, uint32_t xe, const uint32_t value) {
    ff_frame_mark(frame, (int32_t)xo, (int32_t)y, (int32_t)xe, (int32_t)y);
    for(uint32_t x = xo; x <= xe; ++x)
        ff_frame_set(frame, x, y, value);
}

/** @brief Draw a filled circle using the midpoint algorithm.
 *  @param frame  Target frame.
 *  @param xc     Centre X.
 *  @param yc     Centre Y.
 *  @param r      Radius in pixels.
 *  @param value  32-bit RGBA colour.
 */
static void ff_draw_circle_f(Frame* frame, uint32_t xc, uint32_t yc, uint32_t r, uint32_t value) {
    ff_frame_mark(frame, (int32_t)xc - (int32_t)r, (int32_t)yc - (int32_t)r,
                  (int32_t)xc + (int32_t)r, (int32_t)yc + (int32_t)r);
    int x = 0;
    int y = r;
    int p = 1 - r;

    while (x <= y) {
        ++x;
        if (p < 0) {
            p = p + 2 * x + 1;
        } 
        else{
            --y;
            p = p + 2 * (x - y) + 1;
        }

        ff_draw_line_h(frame, yc + x, xc - y, xc + y, value);
        ff_draw_line_h(frame, yc - x, xc - y, xc + y, value);
        ff_draw_line_h(frame, yc + y, xc - x, xc + x, value);
        ff_draw_line_h(frame, yc - y, xc - x, xc + x, value);
    }

    ff_draw_line_h(frame, yc, xc - r, xc + r, value);
}

/** @brief Draw a circle outline using the midpoint algorithm.
 *  @param frame  Target frame.
 *  @param xc     Centre X.
 *  @param yc     Centre Y.
 *  @param r      Radius in pixels.
 *  @param value  32-bit RGBA colour.
 */
static inline void ff_draw_circle_o(Frame* frame, uint32_t xc, uint32_t yc, uint32_t r, uint32_t value) {
    ff_frame_mark(frame, (int32_t)xc - (int32_t)r, (int32_t)yc - (int32_t)r,
                  (int32_t)xc + (int32_t)r, (int32_t)yc + (int32_t)r);
    int x = 0;
    int y = r;
    int p = 1 - r;

    while (x <= y) {
        ++x;
        if (p < 0) {
            p = p + 2 * x + 1;
        } 
        else{
            --y;
            p = p + 2 * (x - y) + 1;
        }

        ff_frame_set(frame, xc + x, yc + y, value);
        ff_frame_set(frame, xc - x, yc + y, value);
        ff_frame_set(frame, xc + x, yc - y, value);
        ff_frame_set(frame, xc - x, yc - y, value);
        ff_frame_set(frame, xc + y, yc + x, value);
        ff_frame_set(frame, xc - y, yc + x, value);
        ff_frame_set(frame, xc + y, yc - x, value);
        ff_frame_set(frame, xc - y, yc - x, value); 
    }

    ff_frame_set(frame, xc, yc - r, value);
    ff_frame_set(frame, xc, yc + r, value);
    ff_frame_set(frame, xc + r, yc, value);
    ff_frame_set(frame, xc - r, yc, value);
}

/** @brief Draw a single 7×8 glyph from a bitmap font.
 *  @param frame   Target frame.
 *  @param font    Font bitmap (7 bytes per glyph, 1 bit per pixel, LSB = top).
 *  @param id      Glyph index.
 *  @param l       Left pixel position.
 *  @param t       Top pixel position.
 *  @param colour  32-bit RGBA colour.
 */
static inline void ff_draw_glyph(Frame* frame, const uint8_t* font, uint32_t id, uint32_t l, uint32_t t, const uint32_t colour) {
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
static inline void ffDrawTextLabel(Frame* frame, const uint8_t* font, const char* text, uint32_t l, uint32_t t, uint32_t, uint32_t, const uint32_t colour) {
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
static inline void ff_sprite_init(Sprite* sprite, uint32_t w, uint32_t h, uint32_t nframes) {
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
static inline void ff_sprite_load_stripe(Sprite* restrict sprite, Frame* restrict frame) {
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
static inline void ff_draw_ltrb_o(Frame* restrict canvas, ltrb32u* restrict r, const uint32_t colour) {
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
static inline void ff_draw_ltrb_f(Frame* restrict canvas, ltrb32u* restrict r, const uint32_t colour)
{
    ff_frame_mark(canvas, (int32_t)r->l, (int32_t)r->t, (int32_t)r->r, (int32_t)r->b);
    for(uint32_t y = r->t; y <= r->b; ++y) {
        for(uint32_t x = r->l; x <= r->r; ++x) {
            ff_frame_set(canvas, x, y, colour);
        }
    }
}

/** Entity Lifecycle **********************************************************************************************************/

/**
 * @brief Create an entity inside a node from a @ref SectorDescriptor.
 * @details Allocates the entity in the node's contiguous array, initialises
 *          the type-specific extension struct, registers the entity in the
 *          stencil layer, and wires up any callback links.
 * @param node        Target node (must have capacity).
 * @param descriptor  Entity blueprint.
 * @return Pointer to the newly created entity.
 */
Entity* ffCreateEntity(Node* restrict node, SectorDescriptor* descriptor) {
    if (node->entities >= node->capacity) {
        node->capacity = node->capacity * 2 + 4;
        node->at = realloc(node->at, node->capacity * sizeof(Entity));
    }
    auto pos = node->entities++;
    auto entity = &node->at[pos];

    entity->index                    = pos;
    entity->uid                      = descriptor->id;
    entity->type                     = descriptor->type;
    entity->core_type                = descriptor->core_type;
    entity->bounds.l                 = descriptor->bounds.l;
    entity->bounds.t                 = descriptor->bounds.t;
    entity->bounds.r                 = descriptor->bounds.l + descriptor->bounds.w;
    entity->bounds.b                 = descriptor->bounds.t + descriptor->bounds.h;
    entity->width                    = descriptor->bounds.w;
    entity->height                   = descriptor->bounds.h;
    entity->value[CP_COARSE]         = (float)(int)descriptor->default_value;
    entity->value[CP_FINE]           = descriptor->default_value - (float)(int)descriptor->default_value;
    entity->repaint                  = true;
    entity->flags                    = descriptor->flags;
    entity->visible                  = true;
    entity->extension                = nullptr;
    entity->parent                   = node;

    switch (descriptor->type) {
        case ST_CHECKBOX:
        case ST_SPRITE_CHECKBOX: {
            entity->extension = (Checkbox*)calloc(1, sizeof(Checkbox));
            auto ext = (Checkbox*)entity->extension;
            ext->radio_id = descriptor->radio_id;
        }
        break;

        case ST_SLIDER: {
            entity->extension = (Slider*)calloc(1, sizeof(Slider));
            auto ext = (Slider*)entity->extension;
            ext->type = descriptor->subtype;
            ext->default_value = descriptor->default_value;
            ext->step[CP_COARSE] = descriptor->step[CP_COARSE];
            ext->step[CP_FINE] = descriptor->step[CP_FINE];
            ext->range[0] = descriptor->range[0];
            ext->range[1] = descriptor->range[1];
            ext->inverse = false;
        }
        break;

        case ST_ENCODER: {
            entity->extension = (Encoder*)calloc(1, sizeof(Encoder));
            auto ext = (Encoder*)entity->extension;
            ext->type = descriptor->subtype;
            ext->default_value = descriptor->default_value;
            ext->step[CP_COARSE] = descriptor->step[CP_COARSE];
            ext->step[CP_FINE] = descriptor->step[CP_FINE];
            ext->range[0] = descriptor->range[0];
            ext->range[1] = descriptor->range[1];
            ext->inverse = false;
        }
        break;

        case ST_ROTARY:
        case ST_SPRITE_INF_SLIDER: {
            entity->extension = (Rotary*)calloc(1, sizeof(Rotary));
            auto ext = (Rotary*)entity->extension;
            ext->default_value = descriptor->default_value;
            ext->step[CP_COARSE] = descriptor->step[CP_COARSE];
            ext->step[CP_FINE] = descriptor->step[CP_FINE];
            ext->range[0] = descriptor->range[0];
            ext->range[1] = descriptor->range[1];
        }
        break;

        case ST_SOCKET: {
            entity->extension = (Socket*)calloc(1, sizeof(Socket));
            auto ext = (Socket*)entity->extension;
            ext->input = nullptr;
            ext->output = nullptr;
            ext->cord = nullptr;
            ext->connection = nullptr;
            ext->connected = false;
            ext->has_data = false;
        }
        break;

        case ST_CRT: {
            entity->extension = (Oscillograph*)calloc(1, sizeof(Oscillograph));
            auto ext = (Oscillograph*)entity->extension;
            auto xi = ff_find_entity_by_id(node->parent, descriptor->input_x);
            auto yi = ff_find_entity_by_id(node->parent, descriptor->input_y);

            ext->x = &((Socket*)xi->extension)->input;
            ext->y = &((Socket*)yi->extension)->input;
            ext->fps = 60;
        }
        break;

        case ST_TEXTBOX: {
            entity->extension = (Textbox*)calloc(1, sizeof(Textbox));
        }
        break;

        case ST_NODE: {
            entity->extension = (EntityNode*)calloc(1, sizeof(EntityNode));
        }
        break;

        case ST_MOMENTARY: {
            entity->extension = (Momentary*)calloc(1, sizeof(Momentary));
        }
        break;

        case ST_SPRITE_BUTTON: {
            entity->extension = (SpriteButton*)calloc(1, sizeof(SpriteButton));
        }
        break;

        default:
        break;

    }

    for(uint32_t i = 0; i < F_CE_LIMIT; ++i) {
        entity->callback[i] = &ff_fuse_link;
    }

    if(!(descriptor->flags & TRANSPARENT)) {
        ff_draw_ltrb_f (
            &node->buffer,
            &entity->bounds,
            entity->index
        );
    }

    init_entity[entity->type](entity);
    if(descriptor->type == ST_TEXTBOX && descriptor->label) {
        auto ext = (Textbox*)entity->extension;
        strncpy(ext->text, descriptor->label, TEXTBOX_SIZE - 1);
        ext->text[TEXTBOX_SIZE - 1] = '\0';
    }

    if(descriptor->callback_target_id) {
        auto target = ff_find_entity_by_id(node->parent, descriptor->callback_target_id);
        if(target) {

            if(descriptor->callback_event == F_CE_REPAINT)
                ff_add_mod_link(entity, target, descriptor->callback_event, ff_set_repaint);
            else
                ff_add_mod_link(entity, target, descriptor->callback_event, ff_value_to_textbox);
        }
    }

    return entity;
}

/**
 * @brief Create a new field (canvas) with all render layers initialised.
 * @details Allocates the field struct, all @ref FieldLayer frame buffers,
 *          the node array (with node 0 as the root canvas entity), and
 *          initialises interaction state.
 * @param x      Left position in screen coordinates.
 * @param y      Top position in screen coordinates.
 * @param w      Width in pixels.
 * @param h      Height in pixels.
 * @param size   Initial node capacity.
 * @param flags  Field flags (@ref FieldFlags).
 * @return Pointer to the newly created field.
 */
Field* ffCreateField(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t size, uint32_t flags) {
    Field *field = malloc(sizeof(Field));

    field->bounds.l     = x;
    field->bounds.t     = y;
    field->bounds.r     = x + w;
    field->bounds.b     = y + h;
    field->width        = w;
    field->height       = h;

    for(int i = 0; i < SP_LIMIT; ++i) {
        field->memory[i] = (point32s) { .x = 0, .y = 0 };
    }

    field->drag         = false;
    field->move         = false;
    field->capacity     = size + 1u;
    field->repaint      = true;
    field->flags        = flags;
    field->step         = 25;
    field->staging      = false;
    field->connecting   = false;
    field->current[IP_NODE] = 0;
    field->current[IP_ENTITY] = 0;
    field->pressed      = nullptr;

    if(true) {
        for(uint32_t i = 0; i < CC; ++i) {
            field->layer[i] = malloc(sizeof(Frame));
            ff_frame_init(field->layer[i],  w, h);
        }

        ff_frame_clr(field->layer[SC]);
        ff_frame_clr(field->layer[SN]);
        ff_frame_fill(field->layer[BG], BACKGROUND);
    }

    field->node = malloc(size * sizeof(Node));
    ff_frame_init(&field->node->buffer, w, h);
    field->nodes = 1;
    field->node[0].entities = 1;
    field->node[0].capacity = size;
    field->node[0].at = malloc(field->node[0].capacity * sizeof(Entity));
    field->node[0].parent = field;
    for(uint32_t i = 0; i < field->node[0].capacity; i++) field->node[0].at[i].index = i;
    field->node[0].index = 0;
    field->node[0].visible = true;

    auto canvas = &field->node[0].at[0];

    canvas->type = ST_CANVAS;
    canvas->parent = &field->node[0];
    canvas->repaint = true;

    for(uint32_t i = 0; i < F_CE_LIMIT; ++i) {
        canvas->callback[i] = &ff_fuse_link;
    }

    return field;
}

/**
 * @brief Destroy a field and free all associated resources.
 * @details Frees all layer frames, all entity extensions (including
 *          type-specific allocations like socket cords, CRT buffers),
 *          node arrays, and the field struct itself.
 * @param field  Field to destroy (must not be used after this call).
 */
void ffDestroyField(Field* restrict field) {
    for(uint32_t i = 0; i < CC; ++i) {
        ff_frame_flush(field->layer[i]);
        free(field->layer[i]);
    }
    for(uint32_t n = 0; n < field->nodes; ++n) {
        for(uint32_t i = 0; i < field->node[n].entities; ++i) {
            auto s = &field->node[n].at[i];
            if(!s->extension) continue;

            switch(s->type) {
                case ST_SOCKET: {
                    auto ext = (Socket*)s->extension;
                    if(ext->cord) free(ext->cord);
                }
                break;

                case ST_NODE: {
                    auto ext = (EntityNode*)s->extension;
                    if(ext->frame) {
                        ff_frame_flush(ext->frame);
                        free(ext->frame);
                    }
                }
                break;

                case ST_CRT: {
                    auto ext = (Oscillograph*)s->extension;
                    if(ext->frame) {
                        ff_frame_flush(ext->frame);
                        free(ext->frame);
                    }
                    if(ext->data) free(ext->data);
                }
                break;

                default:
                break;
            }

            free(s->extension);
        }
        free(field->node[n].at);
    }
    free(field->node);
}


/**
 * @brief Create a new (hidden) node inside a field.
 * @details The node is created invisible; call ffPlaceNode() to position
 *          and show it on the canvas.
 * @param field  Owning field.
 * @param w      Node width in pixels.
 * @param h      Node height in pixels.
 * @param size   Initial entity capacity.
 * @return Pointer to the newly created node.
 */
static inline Node* ffCreateNode(Field* restrict field, uint32_t w, uint32_t h, size_t size) {

    auto index = field->nodes;

    if (index >= field->capacity) {
        field->capacity = field->capacity * 2 + 4;
        field->node = realloc(field->node, field->capacity * sizeof(Node));
    }

    field->node[index].width = w;
    field->node[index].height = h;
    field->node[index].capacity = size;
    field->node[index].entities = 0;
    field->node[index].parent = field;
    field->node[index].index = index;
    field->node[index].visible = false;
    
    field->node[index].at = malloc(size * sizeof(Entity));
    ff_frame_init(&field->node[index].buffer,  w, h);
    
    ++field->nodes;
    return &field->node[index];
}

/**
 * @brief Place a node on the canvas at the given position (makes it visible).
 * @details Copies the node's buffer into the stencil layers, translates
 *          all child entity bounds to screen space, and marks everything
 *          for repaint.
 * @param field  Owning field.
 * @param node   Node to place.
 * @param x      Left position in screen coordinates.
 * @param y      Top position in screen coordinates.
 */
static inline void ffPlaceNode(Field* restrict field, Node* restrict node, uint32_t x, uint32_t y) {
    node->visible = true;
    ff_draw_ltrb_f(node->parent->layer[SN], &(ltrb32u) { .l = x, .t = y, x + node->width, y + node->height }, node->index);
    ff_frame_copy_at(node->parent->layer[SC], &node->buffer, x, y);
    /* SN changed — the canvas grid is drawn around node areas and must be
     * redrawn. Node 0, entity 0 is the root canvas (see ffCreateField). */
    field->node[0].at[0].repaint = true;

    for(uint32_t i = 0; i < node->entities; ++i) {
        auto bounds = &node->at[i].bounds;
        bounds->l += x;
        bounds->t += y;
        bounds->r += x;
        bounds->b += y;

        node->at[i].repaint = true;
    }
    field->repaint = true;
}

/** Entity Utility ************************************************************************************************************/

/**
 * @brief Find an entity by its unique ID across all nodes in a field.
 * @param field  Field to search.
 * @param id     Entity UID to find (0 returns @c NULL immediately).
 * @return Pointer to the entity, or @c NULL if not found.
 */
Entity* ff_find_entity_by_id(Field* restrict field, uint32_t id) {
    if(!id) return nullptr;

    for(uint32_t n = 0; n < field->nodes; ++n) {
        for(uint32_t i = 0; i < field->node[n].entities; ++i) {
            if(field->node[n].at[i].uid == id) return &field->node[n].at[i];
        }
    }
    return nullptr;
}
/** @brief No-op placeholder (reserved for future use). */
void ff_empty() {}

/** @brief No-op callback — default fuse for unused callback slots.
 *  @param a  Ignored.
 *  @param b  Ignored.
 */
void ff_fuse_link(Entity*, Entity*) {}

/**
 * @brief Move an entity to new bounds, updating the foreground and stencil layers.
 * @param entity  Entity to move.
 * @param bounds  New screen-space bounding box.
 */
void ff_move_entity(Entity* restrict entity, ltrb32u* restrict bounds) {
    auto field = entity->parent->parent;
    ff_draw_ltrb_f(field->layer[FG], &entity->bounds, 0x0);
    if(!(entity->flags & TRANSPARENT)) {
        ff_draw_ltrb_f(field->layer[SC], &entity->bounds, 0x0);
        ff_draw_ltrb_f(field->layer[SC], bounds, entity->index);
    }
    entity->bounds = *bounds;
}

/**
 * @brief Register a modular link (callback) from source to target entity.
 * @param source  Source entity (fires the callback).
 * @param target  Target entity (receives the callback).
 * @param type    Callback event type (@ref CallbackEvent).
 * @param fn      Callback function pointer.
 */
void ff_add_mod_link(Entity* source, Entity* target, CallbackEvent type, void (*fn)(Entity*, Entity*)) {
    source->callback[type] = fn;
    source->target[type] = target;
}

/**
 * @brief Set the text content of a textbox entity.
 * @param entity  Textbox entity (must be of type ST_TEXTBOX).
 * @param text    Null-terminated text (truncated to TEXTBOX_SIZE-1 chars).
 */
void ff_set_text_data(Entity* restrict entity, const char* restrict text) {
    auto ext = (Textbox*)entity->extension;
    strncpy(ext->text, text, TEXTBOX_SIZE - 1);
    ext->text[TEXTBOX_SIZE - 1] = '\0';
    entity->repaint = true;
}

/** @brief Copy a slider's value into a textbox as a formatted string (2 decimal places).
 *  @param slider  Source slider/encoder entity.
 *  @param tbox    Target textbox entity.
 */
static inline void ff_value_to_textbox(Entity* slider, Entity* tbox) {
    int precision = 2;
    auto ext = (Textbox*)tbox->extension;
    char *text = ext->text;
    float val = slider->value[CP_COARSE] + slider->value[CP_FINE];
    snprintf(text, TEXTBOX_SIZE, "%.*f", precision, val);
    tbox->repaint = true;
}

/** @brief Mark the target entity for repaint (callback helper).
 *  @param _src    Ignored source entity.
 *  @param target  Entity to mark dirty.
 */
static inline void ff_set_repaint(Entity*, Entity* target) {
    target->repaint = true;
}

/** Hit Test ******************************************************************************************************************/

/**
 * @brief Handle mouse-button press: hit-test, snapshot state, dispatch press.
 * @details Reads the stencil layers to find the entity under the cursor,
 *          stores cursor positions in Field::memory, snapshots slider/encoder
 *          values, and dispatches the type-specific press handler.
 * @param field   Target field.
 * @param x       Mouse X coordinate.
 * @param y       Mouse Y coordinate.
 * @param button  Button/modifier bitmask (@ref ControlModifier).
 */
void ff_hit_test_down(Field* restrict field, int x, int y, uint32_t button) {
    auto uide = ff_frame_get(field->layer[SC], x, y);
    auto uidn = ff_frame_get(field->layer[SN], x, y);
    field->current[IP_NODE] = uidn;
    field->current[IP_ENTITY] = uide;
    if(uidn >= field->nodes || uide >= field->node[uidn].entities) return;
    auto p = &field->node[uidn].at[uide];
    if(!p) return;

    field->pressed = p;
    field->memory[SP_LT_PRESS].x = p->bounds.l;
    field->memory[SP_LT_PRESS].y = p->bounds.t;

    field->memory[SP_CURSOR_PRESS].x = x;
    field->memory[SP_CURSOR_PRESS].y = y;

    field->memory[SP_CURSOR_PRIOR].x = x;
    field->memory[SP_CURSOR_PRIOR].y = y;

    // Snapshot value at drag start for slider/encoder types
    switch(p->type) {
        case ST_SLIDER: {
            auto ext = (Slider*)p->extension;
            ext->memory[CP_COARSE] = p->value[CP_COARSE];
            ext->memory[CP_FINE] = p->value[CP_FINE];
        }
        break;
        case ST_ENCODER: {
            auto ext = (Encoder*)p->extension;
            ext->memory[CP_COARSE] = p->value[CP_COARSE];
            ext->memory[CP_FINE] = p->value[CP_FINE];
        }
        break;
        default:
        break;
    }

    if(p->flags & INTERCON) {
        if(button == RMB) {
            ff_disconnect(field->pressed);
        }
        else
            field->connecting = true;
    }

    if (p->flags & MOVEABLE) {
        if (button == LMB) field->drag = true;
        field->staging = true;
        if(p->type == ST_NODE) {
            auto ext = (EntityNode*)p->extension;
            ext->staging = true;
            ff_draw_ltrb_f(
                p->parent->parent->layer[SN],
                &p->bounds,
                0x0
            );
            /* SN changed — restore the canvas grid under the node's old area. */
            field->node[0].at[0].repaint = true;
        }

    }
    else {
        set_entity[p->type](field->pressed, x, y);
    }
    field->repaint = true;

    p->callback[F_CE_PRESS](p, p->target[F_CE_PRESS]);
}

/** Debug Overlay *************************************************************************************************************/

#ifdef DEBUG_OVERLAY
/** @brief Draw the debug overlay with entity diagnostic information.
 *  @details Renders entity bounds, type, UID, value, flags, and socket/CRT
 *           state as text on the DO layer.
 *  @param field   Target field.
 *  @param entity  Entity under the cursor (may be @c NULL).
 */
static inline void ff_draw_debug_overlay(Field* field, Entity* entity) {
    constexpr uint32_t colour = 0xFF'FF'FF'90;
    ff_frame_fill(field->layer[DO], 0);
    uint32_t voffset = 10;
    uint32_t vstep = 12;
    uint32_t row = 0;

    ffDrawTextLabel(field->layer[DO], gtFont,    "#", 10, voffset + vstep * row++, 100, 10, colour);

    if(!entity) return;

    // ff_frame_copy_with_alpha(field->layer[DO], field->layer[SC], 0x30);

    ff_draw_rect_o(field->layer[DO], 
            entity->bounds.l,
            entity->bounds.t, 
            entity->bounds.r,
            entity->bounds.b, 
            ERROR);

    char buffer[128];
    snprintf(buffer, 128, "CONTROL INDEX: %u", entity->index);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "CORE TYPE    : %u", entity->core_type);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "CONTROL UID  : %x", entity->uid);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "NODE INDEX   : %u", entity->parent->index);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "NODE UID     : %x", entity->parent->uid);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    if(entity->type == ST_SOCKET) {
        auto ext = (Socket*)entity->extension;
        snprintf(buffer, 128, "IS CONNECTED : %d", ext->connected);
        ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
        snprintf(buffer, 128, "CONNECTION   : %p", (void*)ext->connection);
        ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
        snprintf(buffer, 128, "HAS DATA     : %d", ext->has_data);
        ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
        if(ext->input) {
            snprintf(buffer, 128, "INPUT        : %f", *ext->input);
            ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
        }
        if(ext->output) {
            snprintf(buffer, 128, "OUTPUT       : %f", *ext->output);
            ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
        }
    }
    snprintf(buffer, 128, "VALUE COARSE : %f", entity->value[0]);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "VALUE FINE   : %f", entity->value[1]);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "FLAGS        : %b", entity->flags);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    if(entity->type == ST_CRT) {
        auto ext = (Oscillograph*)entity->extension;
        if(ext->x && *ext->x) {
            snprintf(buffer, 128, "X            : %f", **ext->x);
            ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
        }
        if(ext->y && *ext->y) {
            snprintf(buffer, 128, "Y            : %f", **ext->y);
            ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
        }
    }
    atomic_store_explicit(&force_repaint, true, memory_order_release);
}
#endif
/**
 * @brief Handle mouse move: detect hover enter/leave and update cursor state.
 * @details Compares the stencil hit with the currently hovered entity and
 *          dispatches enter/leave handlers when it changes. The leave target
 *          is the previous @c current[] entry — the entity that was actually
 *          hovered — so it is always un-hovered (the old prior[] copy lagged
 *          one change behind and left stale hover highlights).
 * @param field  Target field.
 * @param x      Mouse X coordinate.
 * @param y      Mouse Y coordinate.
 */
void ff_hit_test(Field* restrict field, int x, int y) {
    auto uid_e = ff_frame_get(field->layer[SC], x, y);
    auto uid_n = ff_frame_get(field->layer[SN], x, y);

    if(field->current[IP_ENTITY] != uid_e || field->current[IP_NODE] != uid_n) {
        auto prev_node = field->current[IP_NODE];
        auto prev_uid  = field->current[IP_ENTITY];
        if(prev_node >= field->nodes || prev_uid >= field->node[prev_node].entities) return;
        if(uid_n >= field->nodes || uid_e >= field->node[uid_n].entities) return;
        auto prev    = &field->node[prev_node].at[prev_uid];
        auto current = &field->node[uid_n].at[uid_e];

        #ifdef DEBUG_OVERLAY
            ff_draw_debug_overlay(field, current);
        #endif

        field->current[IP_ENTITY] = uid_e;
        field->current[IP_NODE]   = uid_n;

        prev->hovered = false;
        prev->repaint = true;
        leave_entity[prev->type](prev, x, y);

        current->hovered = true;
        current->repaint = true;
        enter_entity[current->type](current, x, y);

        field->repaint = true;
    }
}

/**
 * @brief Handle mouse drag: forward to the pressed entity's drag handler.
 * @param field  Target field.
 * @param x      Mouse X coordinate.
 * @param y      Mouse Y coordinate.
 */
void ff_hit_test_drag(Field* restrict field, int x, int y) {
    drag_entity[field->pressed->type](field->pressed, x, y);
    field->repaint = true;
}

/**
 * @brief Handle mouse-button release: dispatch release handler and clear drag state.
 * @param field   Target field.
 * @param x       Mouse X coordinate.
 * @param y       Mouse Y coordinate.
 * @param button  Button/modifier bitmask (@ref ControlModifier).
 */
void ff_hit_test_up(Field* restrict field, int x, int y, uint32_t button) {
    auto uid_e = ff_frame_get(field->layer[SC], x, y);
    auto uid_n = ff_frame_get(field->layer[SN], x, y);
    field->current[IP_NODE] = uid_n;
    field->current[IP_ENTITY] = uid_e;

    if(field->pressed) {
        release_entity[field->pressed->type](field->pressed, x, y);
        field->pressed = nullptr;
    }
    else {
 //       if(uid_n >= field->nodes || uid_e >= field->node[uid_n].entities) return;
        auto s = &field->node[uid_n].at[uid_e];
        if(!s) return;
        release_entity[s->type](s, x, y);
    }

    field->drag = false;
    field->move = false;
    field->connecting = false;
    field->repaint = true;

    switch (button) {
        case LMB:
            break;
        case MMB:
            break;
        case RMB:
            break;
        default:
            break; 
    } 
}

/** Entity Handlers ***********************************************************************************************************/

/** @brief Draw a checkbox widget (filled when checked).
 *  @param entity  Checkbox entity to draw.
 */
static void ff_draw_checkbox(Entity* restrict entity) {
    auto field = entity->parent->parent;
    ff_draw_ltrb_f(field->layer[FG], &entity->bounds, BUTTONS);
    ff_draw_ltrb_o(field->layer[FG], &entity->bounds, BORDER);
    if(entity->value[CP_COARSE] > 0.5f)
        ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, entity->bounds.t + GAP, entity->bounds.r - GAP, entity->bounds.b - GAP, SELECTIONBACKGROUND);
    entity->repaint = false;
}

/**
 * @brief Draw all dirty entities across all visible nodes.
 * @details Iterates every node and entity, calling the type-specific draw
 *          handler for entities with the repaint flag set, then fires the
 *          F_CE_REPAINT callback. Clears the global repaint flag afterwards.
 * @param field  Field to render.
 */
void ff_draw_scene(Field* restrict field) {
    for(uint32_t n = 0; n < field->nodes; ++n) {
        if(!field->node[n].visible) continue;
        for(uint32_t i = 0; i < field->node[n].entities; ++i) {
            auto entity = &field->node[n].at[i];
            if(entity->repaint) {
                draw_entity[entity->type](entity);
                entity->callback[F_CE_REPAINT](entity, entity->target[F_CE_REPAINT]);
            }
        }
    }
    field->repaint = false;
}

/** @brief Toggle a checkbox on press (handles radio-group exclusivity).
 *  @param entity  Checkbox entity.
 *  @param x       Mouse X (unused).
 *  @param y       Mouse Y (unused).
 */
static void ff_set_checkbox(Entity* restrict entity, int, int) {
    auto coarse = &entity->value[CP_COARSE];
    auto ext = (Checkbox*)entity->extension;
    if(entity->flags & RADIO) {
        auto id = ext->radio_id;
        if(*coarse > 0.5f) return;
        else {
            *coarse = 1.0f;
            entity->repaint = true;
            auto field = entity->parent->parent;
            for(uint32_t n = 0; n < field->nodes; ++n) {
                for(uint32_t i = 0; i < field->node[n].entities; ++i) {
                    auto r = &field->node[n].at[i];
                    if((r->flags & RADIO) && ext->radio_id == id && r != entity) {
                        r->value[CP_COARSE] = 0.0f;
                        r->repaint = true;
                        printf("Turned off : %d\n", r->index);
                    }
                }
            }
        }
    }
    else {
        if (*coarse < 0.5f) *coarse = 1.0f;
        else *coarse = 0.0f;
    }
    entity->repaint = true;
}

/*** Slider *******************************************************************************************************************/

/** @brief Set a slider's value from a mouse drag position.
 *  @details Uses the cursor delta since the last event, scaled by the step size,
 *          and clamps to the configured range.
 *  @param entity  Slider entity.
 *  @param x       Mouse X coordinate.
 *  @param y       Mouse Y coordinate.
 */
static void ff_set_slider(Entity* restrict entity, int x, int y) {
    const float v = 0.1f;
    const auto coarse = &entity->value[CP_COARSE];
    const auto fine = &entity->value[CP_FINE];
    auto ext = (Slider*)entity->extension;
    auto field = entity->parent->parent;

    float value = {};

    if(entity->flags & VERTICAL) {
        const int dy = field->memory[SP_CURSOR_PRIOR].y - y;
        float inverse = ext->inverse ? -1.0f : 1.0f;
        const auto step = ext->step[CP_COARSE] * inverse;
        value = step * roundf((float)dy * v) + ext->memory[CP_COARSE];
    }
    else {
        const int dx = field->memory[SP_CURSOR_PRIOR].x - x;
        float inverse = ext->inverse ? -1.0f : 1.0f;
        const auto step = ext->step[CP_COARSE] * inverse;
        value = - step * roundf((float)dx * v) + ext->memory[CP_COARSE];
    }

    if(value < ext->range[0]) {
        *coarse = ext->range[0];
        *fine = 0.0f;
    }
    else if(value > ext->range[1]) {
        *coarse = ext->range[1];
        *fine = 0.0f;
    }
    else *coarse = value;

    entity->repaint = true;
    entity->callback[F_CE_VALUE](entity, entity->target[F_CE_VALUE]);
}

/** @brief Draw a slider widget (supports subtypes SS_A, SS_B, SS_C).
 *  @param entity  Slider entity to draw.
 */
static void ff_draw_slider(Entity* restrict entity) {
    auto ext = (Slider*)entity->extension;
    auto coarse = &entity->value[CP_COARSE];
    auto fine = &entity->value[CP_FINE];
    auto field = entity->parent->parent;
    ff_draw_ltrb_f(field->layer[FG], &entity->bounds, BUTTONS);
    ff_draw_ltrb_o(field->layer[FG], &entity->bounds, BORDER);

    float frac = (*coarse - ext->range[0]) / (ext->range[1] - ext->range[0]);

    switch(ext->type) {
        case SS_A:
            if(entity->flags & VERTICAL) {
                int h   = entity->bounds.b - entity->bounds.t - GRIP * 2 - GAP * 2;
                int pos = (int)((1.0f - frac) * (float)h) + entity->bounds.t + GRIP + GAP;
                ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, pos - GRIP,
                            entity->bounds.r - GAP, pos + GRIP, HIGHLIGHT);
            } else {
                int w   = entity->bounds.r - entity->bounds.l - GRIP * 2 - GAP * 2;
                int pos = (int)(frac * (float)w) + entity->bounds.l + GRIP + GAP;
                ff_draw_rect_f(field->layer[FG], pos - GRIP, entity->bounds.t + GAP,
                            pos + GRIP, entity->bounds.b - GAP, HIGHLIGHT);
            }
            break;

        case SS_B:
            if(entity->flags & VERTICAL) {
                int h   = entity->bounds.b - entity->bounds.t - GAP * 2;
                int pos = (int)((1.0f - frac) * (float)h) + entity->bounds.t + GAP;
                ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, pos,
                            entity->bounds.r - GAP, entity->bounds.b - GAP, HIGHLIGHT);
            } else {
                int w   = entity->bounds.r - entity->bounds.l - GAP * 2;
                int pos = (int)(frac * (float)w) + entity->bounds.l + GAP;
                ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, entity->bounds.t + GAP,
                            pos, entity->bounds.b - GAP, HIGHLIGHT);
            }
            break;

        case SS_C: {
            float range_span = ext->range[1] - ext->range[0];
            float zero_frac = (0.0f - ext->range[0]) / range_span;
            float frac = (*coarse - ext->range[0]) / range_span;
            if(entity->flags & VERTICAL) {
                int h   = entity->bounds.b - entity->bounds.t - GAP * 2;
                int cy = entity->bounds.t + GAP + (int)((1.0f - zero_frac) * (float)h);
                int vy = entity->bounds.t + GAP + (int)((1.0f - frac) * (float)h);
                if(vy < cy) {
                    ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, vy,
                                entity->bounds.r - GAP, cy, HIGHLIGHT);
                } 
                else if(vy > cy) {
                    ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, cy,
                                entity->bounds.r - GAP, vy, HIGHLIGHT);
                }
            } 
            else {
                int w   = entity->bounds.r - entity->bounds.l - GAP * 2;
                int cx = entity->bounds.l + GAP + (int)(zero_frac * (float)w);
                int vx = entity->bounds.l + GAP + (int)(frac * (float)w);
                if(vx > cx) {
                    ff_draw_rect_f(field->layer[FG], cx + 1, entity->bounds.t + GAP,
                                vx, entity->bounds.b - GAP, HIGHLIGHT);
                } 
                else if(vx < cx) {
                    ff_draw_rect_f(field->layer[FG], vx, entity->bounds.t + GAP,
                                cx - 1, entity->bounds.b - GAP, HIGHLIGHT);
                }
            }
            break;
        }

        case SS_LIMIT:
        default:
            break;
    }
    entity->repaint = false;
}

/** @brief Adjust a slider's fine value via mouse scroll.
 *  @param entity  Slider entity.
 *  @param x       Scroll delta X (unused).
 *  @param y       Scroll delta Y.
 */
static void ff_scroll_slider(Entity* restrict entity, int x, int y) {
    auto fine = &entity->value[CP_FINE];
    auto coarse = &entity->value[CP_COARSE];
    auto ext = (Slider*)entity->extension;
    float inverse = ext->inverse ? -1.0f : 1.0f;
    auto df = (float)y * ext->step[CP_FINE] * inverse;

    if((*coarse + df < ext->range[1]) && (*coarse + df > ext->range[0])) {
        *fine += df;
    }

    if((int)*fine != 0) {
        *coarse += (int)*fine;
        *fine = 0.0f;
    };

    entity->repaint = true;
    entity->callback[F_CE_VALUE](entity, entity->target[F_CE_VALUE]);
}

/*** Slider *******************************************************************************************************************/

/** @brief Set an encoder's value from a mouse drag (wraps at range edges).
 *  @param entity  Encoder entity.
 *  @param x       Mouse X coordinate.
 *  @param y       Mouse Y coordinate.
 */
static void ff_set_encoder(Entity* restrict entity, int x, int y) {
    const float v = 0.1f;
    const auto coarse = &entity->value[CP_COARSE];
    const auto fine = &entity->value[CP_FINE];
    auto ext = (Encoder*)entity->extension;
    auto field = entity->parent->parent;

    float value = {};

    if(entity->flags & VERTICAL) {
        const int dy = field->memory[SP_CURSOR_PRIOR].y - y;
        float inverse = ext->inverse ? -1.0f : 1.0f;
        const auto step = ext->step[CP_COARSE] * inverse;
        value = step * roundf((float)dy * v) + ext->memory[CP_COARSE];
    }
    else {
        const int dx = field->memory[SP_CURSOR_PRIOR].x - x;
        float inverse = ext->inverse ? -1.0f : 1.0f;
        const auto step = ext->step[CP_COARSE] * inverse;
        value = - step * roundf((float)dx * v) + ext->memory[CP_COARSE];
    }

    if(value < ext->range[0]) {
        ext->inverse = !ext->inverse;
        ext->memory[CP_COARSE] = ext->range[0];
        field->memory[SP_CURSOR_PRIOR].x = x;
        field->memory[SP_CURSOR_PRIOR].y = y;
        *coarse = ext->range[0];
    }
    else if(value > ext->range[1]) {
        ext->inverse = !ext->inverse;
        ext->memory[CP_COARSE] = ext->range[1];
        field->memory[SP_CURSOR_PRIOR].x = x;
        field->memory[SP_CURSOR_PRIOR].y = y;
        *coarse = ext->range[1];
    }
    else *coarse = value;

    entity->repaint = true;
    entity->callback[F_CE_VALUE](entity, entity->target[F_CE_VALUE]);
}

/** @brief Draw an encoder widget (bar fill, respecting inverse state).
 *  @param entity  Encoder entity to draw.
 */
static void ff_draw_encoder(Entity* restrict entity) {
    auto ext = (Encoder*)entity->extension;
    auto coarse = &entity->value[CP_COARSE];
    auto fine = &entity->value[CP_FINE];
    auto field = entity->parent->parent;
    ff_draw_ltrb_f(field->layer[FG], &entity->bounds, BUTTONS);
    ff_draw_ltrb_o(field->layer[FG], &entity->bounds, BORDER);

    float frac = (*coarse - ext->range[0]) / (ext->range[1] - ext->range[0]);

    if(entity->flags & VERTICAL) {
        int h   = entity->bounds.b - entity->bounds.t - GAP * 2;
        if(ext->inverse) {
            frac = 1.0f - frac;
            int pos = (int)((1.0f - frac) * (float)h) + entity->bounds.t + GAP;
            ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, entity->bounds.t + GAP,
                        entity->bounds.r - GAP, pos, HIGHLIGHT);
        } else {
            int pos = (int)((1.0f - frac) * (float)h) + entity->bounds.t + GAP;
            ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, pos,
                        entity->bounds.r - GAP, entity->bounds.b - GAP, HIGHLIGHT);
        }
    } 
    else {
        int w   = entity->bounds.r - entity->bounds.l - GAP * 2;
        int pos = (int)(frac * (float)w) + entity->bounds.l + GAP;
        if(ext->inverse) {
            frac = 1.0f - frac;
            pos = (int)(frac * (float)w) + entity->bounds.l + GAP;
            ff_draw_rect_f(field->layer[FG], 
                        pos, 
                        entity->bounds.t + GAP,
                        entity->bounds.r - GAP, 
                        entity->bounds.b - GAP,
                        HIGHLIGHT);
        }
        else {
            ff_draw_rect_f(field->layer[FG], 
                        entity->bounds.l + GAP, 
                        entity->bounds.t + GAP,
                        pos, 
                        entity->bounds.b - GAP,
                        HIGHLIGHT);
        }
    }

    entity->repaint = false;
}

/** @brief Adjust an encoder's value via mouse scroll (wraps at range edges).
 *  @param entity  Encoder entity.
 *  @param x       Mouse X (used for cursor tracking).
 *  @param y       Scroll delta.
 */
static void ff_scroll_encoder(Entity* restrict entity, int x, int y) {
    const auto coarse = &entity->value[CP_COARSE];
    const auto fine = &entity->value[CP_FINE];
    auto ext = (Encoder*)entity->extension;
    auto field = entity->parent->parent;

    float value = {};

    if(entity->flags & VERTICAL) {
        const int dy = y;
        float inverse = ext->inverse ? -1.0f : 1.0f;
        const auto step = ext->step[CP_FINE] * inverse;
        value = step * dy + *coarse;
    }
    else {
        const int dx = y;
        float inverse = ext->inverse ? -1.0f : 1.0f;
        const auto step = ext->step[CP_FINE] * inverse;
        value = step * dx + *coarse;
    }

    if(value < ext->range[0]) {
        ext->inverse = !ext->inverse;
        ext->memory[CP_COARSE] = ext->range[0];
        field->memory[SP_CURSOR_PRIOR].x = x;
        field->memory[SP_CURSOR_PRIOR].y = y;
        *coarse = ext->range[0];
    }
    else if(value > ext->range[1]) {
        ext->inverse = !ext->inverse;
        ext->memory[CP_COARSE] = ext->range[1];
        field->memory[SP_CURSOR_PRIOR].x = x;
        field->memory[SP_CURSOR_PRIOR].y = y;
        *coarse = ext->range[1];
    }
    else *coarse = value;

    entity->repaint = true;
    entity->callback[F_CE_VALUE](entity, entity->target[F_CE_VALUE]);
}

/** @brief Draw a button widget (highlights on hover, accent when active).
 *  @param entity  Button entity to draw.
 */
static void ff_draw_button(Entity* restrict entity) {
    auto field = entity->parent->parent;
    ff_draw_ltrb_f(field->layer[FG], &entity->bounds, BUTTONS);
    ff_draw_ltrb_o(field->layer[FG], &entity->bounds, BORDER);

    if(entity->hovered)
        ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, entity->bounds.t + GAP, entity->bounds.r - GAP, entity->bounds.b - GAP, ACTIVE);

    if(entity->value[CP_COARSE] > 0.5f)
        ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, entity->bounds.t + GAP, entity->bounds.r - GAP, entity->bounds.b - GAP, ACCENT);

    entity->repaint = false;
}

/** @brief Draw a rotary knob (selects sprite frame based on value).
 *  @param entity  Rotary entity to draw.
 */
static void ff_draw_rotary(Entity* restrict entity) {
    auto field = entity->parent->parent;
    auto ext = (Rotary*)entity->extension;
    auto range = ext->range[1] - ext->range[0];
    Sprite* spr = ext->sprite;
    int f = ((entity->value[CP_COARSE] - ext->range[0]) / range) * spr->nframes;
    ff_frame_copy_at(field->layer[FG], &spr->data[f], entity->bounds.l, entity->bounds.t);

    entity->repaint = false;
}

/** @brief Set a rotary knob's value from mouse drag (clamped to range).
 *  @param entity  Rotary entity.
 *  @param x       Mouse X coordinate.
 *  @param y       Mouse Y coordinate.
 */
static void ff_set_rotary(Entity* restrict entity, int x, int y) {
    auto field = entity->parent->parent;
    auto coarse = &entity->value[CP_COARSE];
    auto ext = (Rotary*)entity->extension;
    int dy = roundf((field->memory[SP_CURSOR_PRIOR].y - y)/ext->step[CP_COARSE]);
    int dx = roundf((x - field->memory[SP_CURSOR_PRIOR].x)/ext->step[CP_COARSE]);

    *coarse += (dy + dx);

    if(*coarse < ext->range[0]) *coarse = ext->range[0];
    else if(*coarse > ext->range[1]) *coarse = ext->range[1];
    entity->repaint = true;

    field->memory[SP_CURSOR_PRIOR].x = (float)x;
    field->memory[SP_CURSOR_PRIOR].y = (float)y;
}

/** @brief Set an infinite slider's value from mouse drag (wraps around range).
 *  @param entity  Infinite-slider entity.
 *  @param x       Mouse X coordinate.
 *  @param y       Mouse Y coordinate.
 */
static void ff_set_sprite_inf_slider(Entity* restrict entity, int x, int y) {
    auto field = entity->parent->parent;
    auto coarse = &entity->value[CP_COARSE];
    auto ext = (Rotary*)entity->extension;
    int dy = roundf((y - field->memory[SP_CURSOR_PRIOR].y)/ext->step[CP_COARSE]);
    int dx = roundf((field->memory[SP_CURSOR_PRIOR].x - x)/ext->step[CP_COARSE]);

    *coarse += (dy + dx);

    if(*coarse < ext->range[0]) *coarse += ext->range[0];
    else if(*coarse > ext->range[1]) *coarse -= ext->range[1];

    entity->repaint = true;
    field->memory[SP_CURSOR_PRIOR].x = (float)x;
    field->memory[SP_CURSOR_PRIOR].y = (float)y;
}

/** @brief Toggle a momentary/sprite button on press.
 *  @param entity  Button entity.
 *  @param x       Mouse X (unused).
 *  @param y       Mouse Y (unused).
 */
static void ff_set_button(Entity* restrict entity, int, int) {
    auto coarse = &entity->value[CP_COARSE];
    if(*coarse < 0.5f) *coarse = 1.0f;
    else *coarse = 0.0f;
    entity->repaint = true;
}

/** @brief Handle button release (only fires if the same button was pressed).
 *  @param entity  Button entity under the cursor at release.
 *  @param x       Mouse X (unused).
 *  @param y       Mouse Y (unused).
 */
static void ff_release_button(Entity* restrict entity, int, int) {
    auto field = entity->parent->parent;
    auto pressed = field->pressed; 
    auto coarse = &pressed->value[CP_COARSE];
    if(pressed) {
        if(pressed == entity) {
            if(*coarse < 0.5f) *coarse = 1.0f;
            else *coarse = 0.0f;
        }
        else *coarse = 0.0f;

        pressed->repaint = true;
    }
}

/** @brief Draw a sprite button (selects frame based on on/off state).
 *  @param entity  Sprite-button or sprite-checkbox entity.
 */
static void ff_draw_sprite_button(Entity* restrict entity) {
    auto field = entity->parent->parent;
    Sprite* spr;
    if(entity->type == ST_SPRITE_CHECKBOX)
        spr = ((Checkbox*)entity->extension)->sprite;
    else
        spr = ((SpriteButton*)entity->extension)->sprite;

    if(entity->value[CP_COARSE] > 0.5f)
        ff_frame_copy_at(field->layer[FG], &spr->data[1], entity->bounds.l, entity->bounds.t);
    else
        ff_frame_copy_at(field->layer[FG], &spr->data[0], entity->bounds.l, entity->bounds.t);

    entity->repaint = false;
}

/** @brief Initialise a socket entity (allocates the Bezier cord buffer).
 *  @param entity  Socket entity to initialise.
 */
static void ff_init_socket(Entity* restrict entity) {
    auto ext = (Socket*)entity->extension;
    ext->cord = (float*)calloc(SPLINE_SEGMENTS * 2, sizeof(float));
}

/** @brief Toggle a socket on press (starts a connection drag).
 *  @param entity  Socket entity.
 *  @param x       Mouse X (unused).
 *  @param y       Mouse Y (unused).
 */
static void ff_set_socket(Entity* restrict entity, int, int) {
    auto coarse = &entity->value[CP_COARSE];
    if(*coarse < 0.5f) *coarse = 1.0f;
    else *coarse = 0.0f;
    entity->repaint = true;
}

/** @brief Draw a socket as a coloured circle (teal=input, red=output).
 *  @param entity  Socket entity to draw.
 */
static void ff_draw_socket(Entity* restrict entity) {
    auto field = entity->parent->parent;
    auto r = entity->width / 2;
    point32u center = {
        .x = entity->bounds.l + r,
        .y = entity->bounds.t + r
    };
    auto color = entity->core_type == F_CT_OUTPUT ? SOCKET_OUT : SOCKET_IN;

    ff_draw_circle_f(field->layer[FG], center.x, center.y, r, color);
    r = entity->width / 4;
    ff_draw_circle_f(field->layer[FG], center.x, center.y, r, BUTTONS);
    entity->repaint = false;
}

/** @brief Evaluate a cubic Bezier curve at parameter @c t.
 *  @param a  Start point (t=0).
 *  @param b  First control point.
 *  @param c  Second control point.
 *  @param d  End point (t=1).
 *  @param t  Parameter (0.0–1.0).
 *  @return Interpolated point on the curve.
 */
static inline point ff_interpolate_bezier(point a, point b, point c, point d, float t)
{
    float ti = 1.0f - t;
    float tsq = t * t;
    float isq = ti * ti;
    
    float fa = isq * ti;
    float fb = 3.0f * t * isq;
    float fc = 3.0f * tsq * ti;
    float fd = tsq * t;
    
    point o = {
        .x = fa * a.x + fb * b.x + fc * c.x + fd * d.x,
        .y = fa * a.y + fb * b.y + fc * c.y + fd * d.y
    };

    return o;
}

/** @brief Drag a socket: update the Bezier cord spline to the cursor.
 *  @details If the socket is already connected, disconnects first.
 *           Computes a cubic Bezier from the socket centre to the cursor.
 *  @param entity  Socket entity being dragged.
 *  @param x       Mouse X coordinate.
 *  @param y       Mouse Y coordinate.
 */
static void ff_drag_socket(Entity* entity, int x, int y)
{
    auto field = entity->parent->parent;
    auto ext = (Socket*)entity->extension;
    if(ext->connected) {
        entity = ext->connection;
        ext = (Socket*)entity->extension;
        field->pressed = entity;
        field->current[IP_ENTITY] = entity->index;
        field->current[IP_NODE] = entity->parent->index;

        auto entity_ext = (Socket*)entity->extension;
        auto conn_ext = (Socket*)entity_ext->connection->extension;

        if(entity_ext->has_data) {
            memset(entity_ext->cord, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
        }
        if(conn_ext->has_data) {
            memset(conn_ext->cord, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
        }
        entity_ext->has_data = false;
        conn_ext->has_data = false;
        conn_ext->connected = false;
        entity_ext->connected = false;

    }

    auto o = entity->parent->parent;

    float xe = (float)x;
    float ye = (float)y;

    if(xe > o->width) xe = (float)o->width;
    else if(xe < 0.0f) xe = 0.0f;

    if(ye > o->height) ye = (float)o->height;
    else if(ye < 0.0f) ye = 0.0f;

    point a, b, c, d;

    float xo = (float)entity->bounds.l + 0.5f * (float)entity->width;
    float yo = (float)entity->bounds.t + 0.5f * (float)entity->height;

    a.x = xo;
    a.y = yo;

    b.x = (xe - xo) * (1.0f/3.0f) + xo;
    c.x = (xe - xo) * (2.0f/3.0f) + xo;

    if(ye < yo) {
        b.y = fabs(yo - ye) * (1.0f - 1.0E-6f) + ye;
        c.y = fabs(yo - ye) * (1.0E-6f) + ye;
    }
    else {
        b.y = fabs(yo - ye) * (1.0E-6f) + yo;
        c.y = fabs(yo - ye) * (1.0f - 1.0E-6f) + yo;
    }

    d.x = xe;
    d.y = ye;

    const float inc = 1.0f / (float)(SPLINE_SEGMENTS - 1) ;
    float t = 0.0;

    auto uva = ff_screen_to_uv(a.x, a.y, o->width, o->height);
    auto uvb = ff_screen_to_uv(b.x, b.y, o->width, o->height);
    auto uvc = ff_screen_to_uv(c.x, c.y, o->width, o->height);
    auto uvd = ff_screen_to_uv(d.x, d.y, o->width, o->height);

    for(uint32_t i = 0, j = 0; i < SPLINE_SEGMENTS; i++) {
        point uvp = ff_interpolate_bezier(uva, uvb, uvc, uvd, t);

        ext->cord[j++] = uvp.x;
        ext->cord[j++] = uvp.y;

        t += inc;
    }

    ext->has_data = true;
}

/** @brief Drag a connected socket's cord to a new target position.
 *  @details Recomputes the Bezier spline for an already-connected socket pair.
 *  @param entity  Socket entity (one end of the connection).
 *  @param x       Target X coordinate.
 *  @param y       Target Y coordinate.
 */
static void ff_drag_cord(Entity* restrict entity, int x, int y)
{
    auto ext = (Socket*)entity->extension;
    if(ext->connected) {
        if(ext->has_data) {
            memset(ext->cord, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
        }
        auto conn_ext = (Socket*)ext->connection->extension;
        if(conn_ext->has_data) {
            memset(conn_ext->cord, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
            conn_ext->has_data = false;
            conn_ext->connected = false;
        }
    }

    auto o = entity->parent->parent;

    float xe = (float)x;
    float ye = (float)y;

    if(xe > o->width) xe = (float)o->width;
    else if(xe < 0.0f) xe = 0.0f;

    if(ye > o->height) ye = (float)o->height;
    else if(ye < 0.0f) ye = 0.0f;

    point a, b, c, d;

    float xo = (float)entity->bounds.l + 0.5f * (float)entity->width;
    float yo = (float)entity->bounds.t + 0.5f * (float)entity->height;

    a.x = xo;
    a.y = yo;

    b.x = (xe - xo) * (1.0f/3.0f) + xo;
    c.x = (xe - xo) * (2.0f/3.0f) + xo;

    if(ye < yo) {
        b.y = fabs(yo - ye) * (1.0f - 1.0E-6f) + ye;
        c.y = fabs(yo - ye) * (1.0E-6f) + ye;
    }
    else {
        b.y = fabs(yo - ye) * (1.0E-6f) + yo;
        c.y = fabs(yo - ye) * (1.0f - 1.0E-6f) + yo;
    }

    d.x = xe;
    d.y = ye;

    const float inc = 1.0f / (float)(SPLINE_SEGMENTS - 1) ;
    float t = 0.0;

    auto uva = ff_screen_to_uv(a.x, a.y, o->width, o->height);
    auto uvb = ff_screen_to_uv(b.x, b.y, o->width, o->height);
    auto uvc = ff_screen_to_uv(c.x, c.y, o->width, o->height);
    auto uvd = ff_screen_to_uv(d.x, d.y, o->width, o->height);

    for(uint32_t i = 0, j = 0; i < SPLINE_SEGMENTS; i++) {
        point uvp = ff_interpolate_bezier(uva, uvb, uvc, uvd, t);

        ext->cord[j++] = uvp.x;
        ext->cord[j++] = uvp.y;

        t += inc;
    }

    ext->has_data = true;
}

/** @brief Connect two socket entities (input ↔ output).
 *  @details Wires the input socket's @c input pointer to the output socket's
 *           @c output pointer. If the target was already connected, the old
 *           connection is cleaned up first.
 *  @param source  Source socket entity.
 *  @param target  Target socket entity.
 *  @return @c true if the connection was established, @c false if the
 *          core types are incompatible (e.g. input→input).
 */
inline static bool ff_connect(Entity* restrict source, Entity* restrict target) {
    auto ext_s = (Socket*)source->extension;
    auto ext_t = (Socket*)target->extension;

    if(source->core_type == F_CT_INPUT && target->core_type == F_CT_OUTPUT) {
        ext_s->input = ext_t->output;
    }
    else if(source->core_type == F_CT_OUTPUT && target->core_type == F_CT_INPUT) {
        ext_t->input = ext_s->output;
    }
    else return false;

    if(ext_t->connected) {
        if(ext_t->has_data) {
            memset(ext_t->cord, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
            ext_t->has_data = false;
            ext_t->connected = false;
        }
        auto prev_conn_ext = (Socket*)ext_t->connection->extension;
        if(prev_conn_ext->has_data) {
            memset(ext_s->cord, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
            prev_conn_ext->has_data = false;
            prev_conn_ext->connected = false;
        }
    }
    ff_drag_cord(source, target->bounds.l + target->width / 2, target->bounds.t + target->height / 2);
    ext_s->connected = true;
    ext_t->connected = true;
    ext_s->connection = target;
    ext_t->connection = source;
    source->hovered = false;
    target->hovered = true;
    source->parent->parent->connecting = false;

    return true;
}

/** @brief Disconnect a socket entity from its current connection.
 *  @details Clears the input pointer (if input socket), resets connection
 *           state on both ends, and frees cord spline data.
 *  @param entity  Socket entity to disconnect.
 */
inline static void ff_disconnect(Entity* restrict entity) {
    auto ext = (Socket*)entity->extension;
    auto target = ext->connection;

    if(entity->core_type == F_CT_INPUT) {
        ext->input = nullptr;
    }
    else if(target && target->core_type == F_CT_INPUT) {
        auto target_ext = (Socket*)target->extension;
        target_ext->input = nullptr;
    }

    if(ext->has_data) {
        memset(ext->cord, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
        ext->has_data = false;
    }
    ext->connected = false;

    if(target) {
        auto target_ext = (Socket*)target->extension;
        if(target_ext->has_data) {
            memset(target_ext->cord, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
        }

        target_ext->connected = false;
        target_ext->has_data = false;
        target_ext->connection = nullptr;
        ext->connection = nullptr;
    }
}

/** @brief Handle socket release: attempt to connect to the entity under the cursor.
 *  @param entity  Socket entity being released.
 *  @param x       Mouse X coordinate.
 *  @param y       Mouse Y coordinate.
 */
static void ff_release_socket(Entity* restrict entity, int x, int y) {
    auto field = entity->parent->parent;
    auto target_uid = ff_frame_get(field->layer[SC], x, y);
    auto target_nid = ff_frame_get(field->layer[SN], x, y);
    if(target_nid >= field->nodes || target_uid >= field->node[target_nid].entities) return;
    auto target = &field->node[target_nid].at[target_uid];

    auto target_ext = (Socket*)target->extension;
    if(target_ext->connected) ff_disconnect(target);
    if(ff_connect(entity, target)) return;
    ff_disconnect(entity);
}

/** @brief Initialise a nested-node entity (allocates the node's frame buffer).
 *  @param entity  Node entity to initialise.
 */
static void ff_init_node(Entity* restrict entity) {
    auto ext = (EntityNode*)entity->extension;
    ext->frame = (Frame*)calloc(1, sizeof(Frame));
    ff_frame_init(ext->frame, entity->width, entity->height);
}

/** @brief Handle node press (prepares for potential drag).
 *  @param entity  Node entity.
 *  @param x       Mouse X (unused).
 *  @param y       Mouse Y (unused).
 */
static void ff_set_node(Entity* restrict entity, int, int) {
    entity->repaint = true;
}

/** @brief Check whether an entity's bounds overlap another node in the stencil.
 *  @param entity  Entity to test.
 *  @return @c true if any pixel in the entity's bounds belongs to a different node.
 */
static inline bool ff_has_overlap(const Entity* restrict entity) {
    auto field = entity->parent->parent;
    const auto l = field->layer[SN];
    for(uint32_t y = entity->bounds.t; y < entity->bounds.b; ++y) {
        for(uint32_t x = entity->bounds.l; x < entity->bounds.r; ++x) {
            auto v = ff_frame_get(l, x, y);
            if(v && v != entity->parent->index)
                return true;
        }
    }
    return false;
}

/** @brief Draw a node background (filled rect with border, or staging outline).
 *  @param entity  Node entity to draw.
 */
static void ff_draw_node(Entity* restrict entity) {
    if(entity->visible) {
        auto field = entity->parent->parent;
        auto ext = (EntityNode*)entity->extension;
        auto l = ext->staging ? ST : NG;

        if(l == ST) {
            ff_frame_clr(field->layer[l]);
            ff_draw_ltrb_o(field->layer[l], &entity->bounds, ff_has_overlap(entity) ? ERROR : HIGHLIGHT);
        }
        else {
            ff_draw_ltrb_f(field->layer[l], &entity->bounds, SECONDBACKGROUND);
            ff_draw_ltrb_o(field->layer[l], &entity->bounds, BORDER);
        }
        entity->repaint = false;
    }
}

/** @brief Drag a node on the canvas (grid-snapped movement).
 *  @param entity  Node entity being dragged.
 *  @param x       Mouse X coordinate.
 *  @param y       Mouse Y coordinate.
 */
static void ff_drag_node(Entity* restrict entity, int x, int y)
{
    auto field = entity->parent->parent;
    bool moved = false;

    int dx = ((x - field->memory[SP_CURSOR_PRIOR].x) / field->step) * field->step;
    {
        int l = ((entity->bounds.l + dx) / field->step) * field->step;
        if((l >= 0) && (l + entity->width <= field->width)) {
            entity->bounds.l = l;
            entity->bounds.r = l + entity->width;
            moved = true;
        }
    }

    int dy = ((y - field->memory[SP_CURSOR_PRIOR].y) / field->step) * field->step;
    {
        int t = ((entity->bounds.t + dy) / field->step) * field->step;
        if((t >= 0) && (t + entity->height <= field->height)) {
            entity->bounds.t = t; 
            entity->bounds.b = t + entity->height;
            moved = true;
        }
    }

    if (moved) {
        
        field->memory[SP_CURSOR_PRIOR].x += dx;
        field->memory[SP_CURSOR_PRIOR].y += dy;
        
        entity->repaint = true;
        field->repaint = true;
    }
}

/** @brief Handle node release: snap back if overlapping, otherwise commit move.
 *  @details If the node overlaps another node, it snaps back to its original
 *           position. Otherwise, all child entities are translated to the
 *           new position and the move is committed.
 *  @param entity  Node entity being released.
 *  @param x       Mouse X (unused).
 *  @param y       Mouse Y (unused).
 */
static void ff_release_node(Entity* restrict entity, int, int)
{
    auto field = entity->parent->parent;
    auto overlap = ff_has_overlap(entity);

    ltrb32u ir = {
        .l = field->memory[SP_LT_PRESS].x, 
        .t = field->memory[SP_LT_PRESS].y, 
        .r = field->memory[SP_LT_PRESS].x + entity->width, 
        .b = field->memory[SP_LT_PRESS].y + entity->height
    };

    if(overlap) { 
        entity->bounds = ir;
        ff_draw_ltrb_f(field->layer[SN], &ir, entity->parent->index);
        ff_draw_ltrb_f(field->layer[NG], &ir, 0x0);

        for(uint32_t i = 0; i < entity->parent->entities; ++i) {
            entity->parent->at[i].repaint = true;
        }
    }
    else {
        ff_draw_ltrb_f(field->layer[SN], &ir, 0x0);
        ff_draw_ltrb_f(field->layer[SN], &entity->bounds, entity->parent->index);

        ff_draw_ltrb_f(field->layer[NG], &ir, 0x0);
        ff_draw_ltrb_f(field->layer[TL], &ir, 0x0);

        int dx = entity->bounds.l - field->memory[SP_LT_PRESS].x;
        int dy = entity->bounds.t - field->memory[SP_LT_PRESS].y;

        for(uint32_t i = 0; i < entity->parent->entities; ++i) {
            auto child = &entity->parent->at[i];
            if(child == entity) continue;

            ltrb32u r = {
                .l = child->bounds.l + dx,
                .t = child->bounds.t + dy,
                .r = child->bounds.r + dx,
                .b = child->bounds.b + dy
            };

            ff_move_entity(child, &r); 

            if(child->type == ST_SOCKET) {
                auto child_ext = (Socket*)child->extension;
                if(child_ext->connected) {
                    auto target = child_ext->connection;
                    auto target_ext = (Socket*)target->extension;
                    if(child_ext->has_data) {
                        ff_drag_cord(child, target->bounds.l + target->width / 2, target->bounds.t + target->height / 2);
                    }
                    else if(target_ext->has_data) {
                        ff_drag_cord(target, child->bounds.l + child->width / 2, child->bounds.t + child->height / 2);
                    }
                }
            }

            child->repaint = true;
        }
    }

    ff_frame_clr(field->layer[ST]);
    field->staging = false;
    auto ext = (EntityNode*)entity->extension;
    ext->staging = false;
    /* SN changed (node committed or snapped back) — redraw the canvas grid. */
    field->node[0].at[0].repaint = true;
    entity->repaint = true;
    field->repaint = true;
}

/** @brief Initialise a textbox entity (no-op — buffer is embedded in the struct).
 *  @param entity  Textbox entity (unused).
 */
static void ff_init_textbox(Entity* restrict entity) {
    // Text buffer is embedded in the Textbox extension, nothing to allocate
    (void)entity;
}

/** @brief Handle textbox press (marks for repaint).
 *  @param entity  Textbox entity.
 *  @param x       Mouse X (unused).
 *  @param y       Mouse Y (unused).
 */
static void ff_set_textbox(Entity* restrict entity, int, int) {
    entity->repaint = true;
}

/** @brief Draw a textbox: renders the text label on the text layer.
 *  @param entity  Textbox entity to draw.
 */
static void ff_draw_textbox(Entity* restrict entity) {
    auto field = entity->parent->parent;
    ff_draw_ltrb_f(field->layer[TL], &entity->bounds, 0);
    auto ext = (Textbox*)entity->extension;
    char *text = ext->text;
    auto offset_y = (entity->height - 8) / 2;

    ffDrawTextLabel(
        field->layer[TL],
        gtFont,
        text,
        entity->bounds.l,
        entity->bounds.t + offset_y,
        0,
        0,
        TEXT
    );

    entity->repaint = false;
}

/** @brief Draw a CRT graticule (grid lines, minor ticks, and dotted reference lines).
 *  @param frame   Target frame.
 *  @param colour  32-bit RGBA colour for the grid.
 *  @param vdiv    Number of major vertical divisions.
 *  @param hdiv    Number of major horizontal divisions.
 */
static void ff_draw_graticule(Frame* frame, uint32_t colour, uint32_t vdiv, uint32_t hdiv) {
    const float W = (float)frame->width;
    const float H = (float)frame->height;

    const float stepX = W / (float)vdiv;
    const float stepY = H / (float)hdiv;
    const float step  = stepX < stepY ? stepX : stepY;

    const float gridW = step * (float)vdiv;
    const float gridH = step * (float)hdiv;

    const float hgap = (W - gridW) * 0.5f;
    const float vgap = (H - gridH) * 0.5f;

    const float cx = hgap + gridW * 0.5f;
    const float cy = vgap + gridH * 0.5f;

    const float sub = step / 5.0f;

    // Major vertical lines
    const uint32_t y0 = (uint32_t)(vgap + 0.5f);
    const uint32_t y1 = (uint32_t)(vgap + gridH - 0.5f);
    for (uint32_t i = 0; i <= vdiv; ++i) {
        auto x = (uint32_t)(hgap + (float)i * step + 0.5f);
        if (x >= frame->width) x = frame->width - 1;
        ff_draw_line_v(frame, x, y0, y1, colour);
    }

    // Major horizontal lines
    const uint32_t x0 = (uint32_t)(hgap + 0.5f);
    const uint32_t x1 = (uint32_t)(hgap + gridW - 0.5f);
    for (uint32_t i = 0; i <= hdiv; ++i) {
        auto y = (uint32_t)(vgap + (float)i * step + 0.5f);
        if (y >= frame->height) y = frame->height - 1;
        ff_draw_line_h(frame, y, x0, x1, colour);
    }

    // Minor vertical ticks
    const float tickH = step * 0.2f;
    const uint32_t ty0 = (uint32_t)(cy - tickH + 0.5f);
    const uint32_t ty1 = (uint32_t)(cy + tickH - 0.5f);
    for (uint32_t i = 0; i < vdiv * 5; ++i) {
        auto x = (uint32_t)(hgap + (float)i * sub + 0.5f);
        if (x >= frame->width) x = frame->width - 1;
        ff_draw_line_v(frame, x, ty0, ty1, colour);
    }

    // Minor horizontal ticks
    const float tickW = step * 0.2f;
    const uint32_t tx0 = (uint32_t)(cx - tickW + 0.5f);
    const uint32_t tx1 = (uint32_t)(cx + tickW - 0.5f);
    for (uint32_t i = 0; i < hdiv * 5; ++i) {
        auto y = (uint32_t)(vgap + (float)i * sub + 0.5f);
        if (y >= frame->height) y = frame->height - 1;
        ff_draw_line_h(frame, y, tx0, tx1, colour);
    }

    // Dotted lines at ±2.4 divisions
    const float dottedOffset = step * 2.4f;
    const auto yt = (size_t)(cy - dottedOffset + 0.5f);
    const auto yb = (size_t)(cy + dottedOffset + 0.5f);

    for (uint32_t i = 0; i < vdiv * 5; ++i) {
        auto x = (size_t)(hgap + (float)i * sub + 0.5f);
        if (x >= frame->width) x = frame->width - 1;
        ff_frame_set(frame, x, yt, colour);
        ff_frame_set(frame, x, yb, colour);
    }
}

/** @brief Initialise a CRT entity (allocates frame, draws graticule, registers stream).
 *  @param entity  CRT entity to initialise.
 */
static void ff_init_crt(Entity* restrict entity) {
    auto ext = (Oscillograph*)entity->extension;
    ext->frame = (Frame*)malloc(sizeof(Frame));
    ff_frame_init(ext->frame, entity->width, entity->height);
    ff_draw_graticule(ext->frame, BACKGROUND, 10, 8);

    ext->data = (float*)calloc(SPLINE_SEGMENTS * 2, sizeof(float));
    ext->r_head = 0;
    ext->w_head = 0;

    entity->parent->parent->stream[0] = entity;
}

/** @brief Handle CRT press (marks for repaint).
 *  @param entity  CRT entity.
 *  @param x       Mouse X (unused).
 *  @param y       Mouse Y (unused).
 */
static void ff_set_crt(Entity* restrict entity, int, int) {
    entity->repaint = true;
}

/** @brief Draw a CRT display (copies the CRT frame buffer to the foreground layer).
 *  @param entity  CRT entity to draw.
 */
static void ff_draw_crt(Entity* restrict entity) {
    auto field = entity->parent->parent;
    auto ext = (Oscillograph*)entity->extension;
    ff_draw_ltrb_f(field->layer[FG], &entity->bounds, BUTTONS);
    ff_frame_copy_at(field->layer[FG], ext->frame, entity->bounds.l, entity->bounds.t);

    entity->repaint = false;
}

/** @brief Draw the canvas background grid (minor and major lines).
 *  @param entity  Canvas entity (node 0, entity 0).
 */
static void ff_draw_canvas(Entity* restrict entity)
{
    auto field = entity->parent->parent;
    ff_frame_fill(field->layer[BG], BACKGROUND);
    uint32_t major = 4;
    if(true) {
            
        for(uint32_t x = field->step, l = 1; x < field->layer[BG]->width; x += field->step) {
            for(uint32_t y = 0; y < field->layer[BG]->height; ++y) {
                if(!ff_frame_get(field->layer[SN], x, y))
                    ff_frame_set(field->layer[BG], x, y, l ? MINOR : MAJOR);
            }
            if(++l >= major) l = 0;
        }
   
        for(uint32_t y = field->step, l = 1; y < field->layer[BG]->height; y += field->step) {
            for(uint32_t x = 0; x < field->layer[BG]->width; ++x) {
                if(!ff_frame_get(field->layer[SN], x, y))
                    ff_frame_set(field->layer[BG], x, y, l ? MINOR : MAJOR);
            }
            if(++l >= major) l = 0;
        }
    }
    /* The grid depends only on the SN stencil, not on hover — it must be
     * redrawn only when SN changes (node placed / drag started / released).
     * Sites that modify SN set this flag; see ffPlaceNode, ff_hit_test_down
     * and ff_release_node. */
    entity->repaint = false;
}

/** Dispatch Tables ***********************************************************************************************************/

/** @brief No-op init handler for entity types that need no initialisation. */
static inline void ff_init_none(Entity* restrict) {}

/**
 * @brief Dispatch table: per-type entity initialisation handlers.
 * @details Indexed by @ref SectorType. Called once when the entity is created.
 */
void (*init_entity[])(Entity* restrict)  = {
    [ST_SLIDER]                 = ff_init_none,
    [ST_ENCODER]                = ff_init_none,
    [ST_ROTARY]                 = ff_init_none,  
    [ST_SPRITE_INF_SLIDER]      = ff_init_none,  
    [ST_SOCKET]                 = ff_init_socket,
    [ST_CHECKBOX]               = ff_init_none,  
    [ST_MOMENTARY]              = ff_init_none,  
    [ST_SPRITE_CHECKBOX]        = ff_init_none,  
    [ST_SPRITE_BUTTON]          = ff_init_none,  
    [ST_CANVAS]                 = ff_init_none,  
    [ST_TEXTBOX]                = ff_init_textbox,  
    [ST_NODE]                   = ff_init_node,  
    [ST_CRT]                    = ff_init_crt,  
};

/** @brief No-op set handler for entity types without press behaviour. */
static inline void ff_set_none(Entity* restrict, int, int) {}

/**
 * @brief Dispatch table: per-type press (mouse-down) handlers.
 * @details Indexed by @ref SectorType.
 */
void (*set_entity[])(Entity* restrict, int, int) = {
    [ST_SLIDER]                 = ff_set_slider,   
    [ST_ENCODER]                = ff_set_encoder,   
    [ST_ROTARY]                 = ff_set_rotary,     
    [ST_SPRITE_INF_SLIDER]      = ff_set_sprite_inf_slider, 
    [ST_SOCKET]                 = ff_set_socket,            
    [ST_CHECKBOX]               = ff_set_checkbox,          
    [ST_MOMENTARY]              = ff_set_button,            
    [ST_SPRITE_CHECKBOX]        = ff_set_checkbox,          
    [ST_SPRITE_BUTTON]          = ff_set_button,            
    [ST_CANVAS]                 = ff_set_none,              
    [ST_TEXTBOX]                = ff_set_textbox,
    [ST_NODE]                   = ff_set_node,       
    [ST_CRT]                    = ff_set_crt        
};

/**
 * @brief Dispatch table: per-type draw handlers.
 * @details Indexed by @ref SectorType. Called during ff_draw_scene() for dirty entities.
 */
void (*draw_entity[])(Entity* restrict) = {
    [ST_SLIDER]                 = ff_draw_slider,        
    [ST_ENCODER]                = ff_draw_encoder,        
    [ST_ROTARY]                 = ff_draw_rotary, 
    [ST_SPRITE_INF_SLIDER]      = ff_draw_rotary, 
    [ST_SOCKET]                 = ff_draw_socket,        
    [ST_CHECKBOX]               = ff_draw_checkbox,      
    [ST_MOMENTARY]              = ff_draw_button,        
    [ST_SPRITE_CHECKBOX]        = ff_draw_sprite_button, 
    [ST_SPRITE_BUTTON]          = ff_draw_sprite_button, 
    [ST_CANVAS]                 = ff_draw_canvas,        
    [ST_TEXTBOX]                = ff_draw_textbox,
    [ST_NODE]                   = ff_draw_node,
    [ST_CRT]                    = ff_draw_crt
};

/** @brief No-op drag handler for entity types that don't support dragging. */
static inline void ff_drag_none(Entity* restrict, int, int) {}

/**
 * @brief Dispatch table: per-type drag handlers.
 * @details Indexed by @ref SectorType. Called during ff_hit_test_drag().
 */
void (*drag_entity[])(Entity* restrict, int, int) = {
    [ST_SLIDER]                 = ff_set_slider,           
    [ST_ENCODER]                = ff_set_encoder,           
    [ST_ROTARY]                 = ff_set_rotary,    
    [ST_SPRITE_INF_SLIDER]      = ff_set_sprite_inf_slider,
    [ST_SOCKET]                 = ff_drag_socket,          
    [ST_CHECKBOX]               = ff_drag_none,             
    [ST_MOMENTARY]              = ff_drag_none,             
    [ST_SPRITE_CHECKBOX]        = ff_drag_none,             
    [ST_SPRITE_BUTTON]          = ff_drag_none,             
    [ST_CANVAS]                 = ff_drag_none,             
    [ST_TEXTBOX]                = ff_drag_none,  
    [ST_NODE]                   = ff_drag_node,          
    [ST_CRT]                    = ff_drag_none,  
};

/** @brief No-op scroll handler for entity types that don't support scroll. */
static inline void ff_scroll_none(Entity* restrict, int, int) {}

/**
 * @brief Dispatch table: per-type scroll (mouse wheel) handlers.
 * @details Indexed by @ref SectorType.
 */
void (*scroll_entity[])(Entity* restrict, int, int) = {
    [ST_SLIDER]                 = ff_scroll_slider,     
    [ST_ENCODER]                = ff_scroll_encoder,     
    [ST_ROTARY]                 = ff_scroll_slider,     
    [ST_SPRITE_INF_SLIDER]      = ff_scroll_slider,     
    [ST_SOCKET]                 = ff_scroll_none,          
    [ST_CHECKBOX]               = ff_scroll_none,          
    [ST_MOMENTARY]              = ff_scroll_none,          
    [ST_SPRITE_CHECKBOX]        = ff_scroll_none,          
    [ST_SPRITE_BUTTON]          = ff_scroll_none,          
    [ST_CANVAS]                 = ff_scroll_none,          
    [ST_TEXTBOX]                = ff_scroll_none,  
    [ST_NODE]                   = ff_scroll_none,
    [ST_CRT]                    = ff_scroll_none,
};

/** @brief No-op hover-enter handler for entity types without hover behaviour. */
static inline void ff_enter_none(Entity* restrict, int, int) {}

/**
 * @brief Dispatch table: per-type hover-enter handlers.
 * @details Indexed by @ref SectorType. Called when the cursor enters an entity.
 */
void (*enter_entity[])(Entity* restrict, int, int) = {
    [ST_SLIDER]                 = ff_enter_none,         
    [ST_ENCODER]                = ff_enter_none,         
    [ST_ROTARY]                 = ff_enter_none,         
    [ST_SPRITE_INF_SLIDER]      = ff_enter_none,         
    [ST_SOCKET]                 = ff_enter_none,         
    [ST_CHECKBOX]               = ff_enter_none,         
    [ST_MOMENTARY]              = ff_enter_none,   
    [ST_SPRITE_CHECKBOX]        = ff_enter_none,         
    [ST_SPRITE_BUTTON]          = ff_enter_none,   
    [ST_CANVAS]                 = ff_enter_none,         
    [ST_TEXTBOX]                = ff_enter_none,  
    [ST_NODE]                   = ff_enter_none,
    [ST_CRT]                    = ff_enter_none,
};

/** @brief No-op hover-leave handler for entity types without hover behaviour. */
static inline void ff_leave_none(Entity* restrict, int, int) {}

/**
 * @brief Dispatch table: per-type hover-leave handlers.
 * @details Indexed by @ref SectorType. Called when the cursor leaves an entity.
 */
void (*leave_entity[])(Entity* restrict, int, int) = {
    [ST_SLIDER]                 = ff_leave_none,         
    [ST_ENCODER]                = ff_leave_none,         
    [ST_ROTARY]                 = ff_leave_none,         
    [ST_SPRITE_INF_SLIDER]      = ff_leave_none,         
    [ST_SOCKET]                 = ff_leave_none,         
    [ST_CHECKBOX]               = ff_leave_none,         
    [ST_MOMENTARY]              = ff_leave_none,   
    [ST_SPRITE_CHECKBOX]        = ff_leave_none,         
    [ST_SPRITE_BUTTON]          = ff_leave_none,   
    [ST_CANVAS]                 = ff_leave_none,         
    [ST_TEXTBOX]                = ff_leave_none,  
    [ST_NODE]                   = ff_leave_none,
    [ST_CRT]                    = ff_leave_none  
};

/** @brief No-op release handler for entity types without release behaviour. */
static inline void ff_release_none(Entity* restrict, int, int) {}

/**
 * @brief Dispatch table: per-type release (mouse-up) handlers.
 * @details Indexed by @ref SectorType. Called during ff_hit_test_up().
 */
void (*release_entity[])(Entity* restrict, int, int) = {
    [ST_SLIDER]                 = ff_release_none,
    [ST_ENCODER]                = ff_release_none,
    [ST_ROTARY]                 = ff_release_none,
    [ST_SPRITE_INF_SLIDER]      = ff_release_none,
    [ST_SOCKET]                 = ff_release_socket,
    [ST_CHECKBOX]               = ff_release_none,
    [ST_MOMENTARY]              = ff_release_button,
    [ST_SPRITE_CHECKBOX]        = ff_release_none,
    [ST_SPRITE_BUTTON]          = ff_release_button,
    [ST_CANVAS]                 = ff_release_none,
    [ST_TEXTBOX]                = ff_release_none,  
    [ST_NODE]                   = ff_release_node,
    [ST_CRT]                    = ff_release_node,
}; 

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
  0x00,0x00,0x03,0x07,0x04,0x00,0x00, // 64 = '
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
