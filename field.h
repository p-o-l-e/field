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
 * - Dispatch tables for per-type init/set/draw/drag/scroll/enter/leave/release.
 *
 * Primitive drawing types and functions (Frame, Sprite, shapes, text, colour
 * utilities) live in geometry.h, which is included below.
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

#include "geometry.h"

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
 * @brief Node anchor flags — which field (window) borders a node is glued to.
 * @details Combine with bitwise OR and assign to @ref Node::anchors. When the
 *          field is resized (see ff_resize_field()), an anchored edge is pinned
 *          to the corresponding window border: the node stretches between two
 *          anchored edges or translates with a single one. NONE keeps the
 *          node's position and size unchanged.
 */
typedef enum: uint32_t {
    NONE        = 0,
    LEFT        = 1 << 0,   ///< Node's left edge follows the window's left border.
    TOP         = 1 << 1,   ///< Node's top edge follows the window's top border.
    RIGHT       = 1 << 2,   ///< Node's right edge follows the window's right border.
    BOTTOM      = 1 << 3,   ///< Node's bottom edge follows the window's bottom border.
    ALL         = LEFT | TOP | RIGHT | BOTTOM   ///< Node fills the window.

} ResizeFlags;

/**
 * @brief Per-field behaviour flags.
 */
typedef enum {
    ROOT        = 1 << 0,   ///< This field is the root field.

} FieldFlags;

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
    float const** input_link;   ///< Bound module's icv slot for this input (set by bridge, updated on connect/disconnect).
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
    void*       data;           ///< Stream ring buffer (size * 2 floats).
    Frame*      frame;          ///< CRT frame buffer (graticule + trace).
    uint32_t    r_head;         ///< Ring buffer valid point count (saturates at size).
    uint32_t    w_head;         ///< Ring buffer write cursor (float index, wraps at size * 2).
    uint32_t    size;           ///< Ring buffer capacity in points (default 256).
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
    uint32_t    anchors;            ///< Window-border anchors (@ref ResizeFlags).
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
    uint32_t    policy_scale_init;  ///< Initial frame-heap capacity multiplier (2 = 2x).
    uint32_t    policy_scale_num;   ///< Frame-heap grow-step numerator (1).
    uint32_t    policy_scale_den;   ///< Frame-heap grow-step denominator (8 = +1/8).
};

/** @brief Create an entity inside a node from a descriptor.
 *  @param node        Target node.
 *  @param descriptor  Entity blueprint (see @ref SectorDescriptor, read-only).
 *  @return Pointer to the newly created entity, or @c NULL on failure.
 */
[[nodiscard]] Entity* ffCreateEntity(Node* restrict, const SectorDescriptor*);

/** @brief Create a new field (canvas).
 *  @param x      Left position in screen coordinates.
 *  @param y      Top position in screen coordinates.
 *  @param w      Width in pixels.
 *  @param h      Height in pixels.
 *  @param size   Initial node capacity.
 *  @param flags  Field flags (@ref FieldFlags).
 *  @return Pointer to the newly created field.
 */
[[nodiscard]] Field*  ffCreateField(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

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
[[nodiscard]] static Node* ffCreateNode(Field* restrict, uint32_t, uint32_t, size_t);

/** @brief Place a node on the canvas at the given position (makes it visible).
 *  @param field  Owning field.
 *  @param node   Node to place.
 *  @param x      Left position in screen coordinates.
 *  @param y      Top position in screen coordinates.
 */
static void  ffPlaceNode(Field* restrict, Node* restrict, uint32_t, uint32_t);

/** @brief Resize the field to new dimensions and re-anchor its nodes.
 *  @details Resizes every render layer (and the root canvas buffer) to the new
 *           size, then resizes/moves every visible node whose
 *           @ref Node::anchors is not NONE so its anchored edges sit on the
 *           corresponding window borders. Called when the window is resized.
 *  @param field  Field to resize.
 *  @param w      New width in pixels.
 *  @param h      New height in pixels.
 */
void    ff_resize_field(Field* restrict, uint32_t, uint32_t);

/** @brief Resize/move a node so its anchored edges sit on the field borders.
 *  @details Computes the node's new screen rectangle from @ref Node::anchors
 *           and the field bounds, then resizes the node's buffers and
 *           stencils and translates its child entities. Anchored edges are
 *           glued to their borders; the opposite edge keeps the node's size.
 *  @param field  Owning field.
 *  @param node   Node to re-anchor (must be visible and have an ST_NODE entity).
 */
static void ff_anchor_node(Field* restrict, Node* restrict);

/** @brief Find an entity by its unique ID across all nodes.
 *  @param field  Field to search.
 *  @param id     Entity UID to find (0 returns @c NULL).
 *  @return Pointer to the entity, or @c NULL if not found.
 */
[[nodiscard]] Entity* ff_find_entity_by_id(Field* restrict, uint32_t);

/** @brief No-op placeholder (reserved). */
void    ff_empty(void);

/** @brief No-op callback — default link fuse for unused callback slots.
 *  @param a  Ignored.
 *  @param b  Ignored.
 */
void    ff_fuse_link([[maybe_unused]] Entity*, [[maybe_unused]] Entity*);

/** @brief Move an entity to new bounds, updating stencil layers.
 *  @param entity  Entity to move.
 *  @param bounds  New bounding box (read-only).
 */
void    ff_move_entity(Entity* restrict, const ltrb32u* restrict);

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
void    ff_set_text_data(Entity* restrict, const char* restrict);

/** @brief Copy a slider's value into a textbox as a formatted string.
 *  @param slider  Source slider entity (read-only).
 *  @param tbox    Target textbox entity.
 */
static void ff_value_to_textbox(Entity* restrict slider, Entity* restrict tbox);

/** @brief Mark the target entity for repaint.
 *  @param _src    Ignored source entity.
 *  @param target  Entity to mark dirty.
 */
static void ff_set_repaint([[maybe_unused]] Entity* restrict _src, Entity* restrict target);

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
 *  @param entity  Entity under the cursor (may be @c NULL, read-only).
 */
static void ff_draw_debug_overlay(Field* restrict, const Entity* restrict);
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
[[nodiscard]] Entity* ffCreateEntity(Node* restrict node, const SectorDescriptor* descriptor) {
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
    /* The Entity array comes from malloc (ffCreateNode) — every field must be
     * initialised here; ff_draw_button reads hovered before the first hit
     * test pass can set it. */
    entity->hovered                  = false;
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
[[nodiscard]] Field* ffCreateField(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t size, uint32_t flags) {
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
    field->policy_scale_init = 2;
    field->policy_scale_num  = 1;
    field->policy_scale_den  = 8;
    field->staging      = false;
    field->connecting   = false;
    field->current[IP_NODE] = 0;
    field->current[IP_ENTITY] = 0;
    field->pressed      = nullptr;
    /* The field is malloc'd — stream[] must be zeroed before first use;
       ff_init_crt only sets stream[0], and readers test every slot. */
    for(size_t i = 0; i < sizeof(field->stream) / sizeof(field->stream[0]); ++i)
        field->stream[i] = nullptr;

    if(true) {
        for(uint32_t i = 0; i < CC; ++i) {
            field->layer[i] = malloc(sizeof(Frame));
            /* Window heap policy: policy_scale_init x the window size at creation
               — resizes within the headroom never reallocate (see
               ff_frame_resize). */
            ff_frame_init_cap(field->layer[i], w, h, (size_t)w * h * field->policy_scale_init);
        }

        ff_frame_clr(field->layer[SC]);
        ff_frame_clr(field->layer[SN]);
        ff_frame_fill(field->layer[BG], BACKGROUND);
    }

    /* capacity is size + 1 (root node + size module nodes) — allocate to match,
       otherwise ffCreateNode writes past the array once all slots are used. */
    field->node = malloc(field->capacity * sizeof(Node));
    /* Root canvas buffer is window-sized — same policy_scale_init policy as the layers. */
    ff_frame_init_cap(&field->node->buffer, w, h, (size_t)w * h * field->policy_scale_init);
    field->nodes = 1;
    field->node[0].entities = 1;
    field->node[0].capacity = size;
    field->node[0].at = malloc(field->node[0].capacity * sizeof(Entity));
    field->node[0].parent = field;
    for(uint32_t i = 0; i < field->node[0].capacity; i++) field->node[0].at[i].index = i;
    field->node[0].index = 0;
    field->node[0].visible = true;
    field->node[0].anchors = NONE;

    auto canvas = &field->node[0].at[0];
    /* The canvas entity is built by hand on malloc'd memory, not via
     * ffCreateEntity — zero it so extension is NULL (ff_release_socket and
     * ffDestroyField both inspect it) and flags/target[] are defined. */
    memset(canvas, 0, sizeof(Entity));

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
        /* The node-local stencil buffer (resized by ff_anchor_node) is owned
         * by the node — release it here. */
        ff_frame_flush(&field->node[n].buffer);
    }
    free(field->node);
    free(field);
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
[[nodiscard]] static inline Node* ffCreateNode(Field* restrict field, uint32_t w, uint32_t h, size_t size) {

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
    field->node[index].anchors = NONE;
    
    field->node[index].at = malloc(size * sizeof(Entity));
    /* Node buffers resize with anchored nodes — same policy_scale_init policy. */
    ff_frame_init_cap(&field->node[index].buffer, w, h, (size_t)w * h * field->policy_scale_init);
    
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

    /* Anchored nodes snap to the window borders immediately at placement. */
    if(node->anchors != NONE)
        ff_anchor_node(field, node);
}

/** Entity Utility ************************************************************************************************************/

/**
 * @brief Find an entity by its unique ID across all nodes in a field.
 * @param field  Field to search.
 * @param id     Entity UID to find (0 returns @c NULL immediately).
 * @return Pointer to the entity, or @c NULL if not found.
 */
[[nodiscard]] Entity* ff_find_entity_by_id(Field* restrict field, uint32_t id) {
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
void ff_fuse_link([[maybe_unused]] Entity* a, [[maybe_unused]] Entity* b) {}

/**
 * @brief Move an entity to new bounds, updating the foreground and stencil layers.
 * @param entity  Entity to move.
 * @param bounds  New screen-space bounding box.
 */
void ff_move_entity(Entity* restrict entity, const ltrb32u* restrict bounds) {
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
static void ff_value_to_textbox(Entity* restrict slider, Entity* restrict tbox) {
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
static inline void ff_set_repaint([[maybe_unused]] Entity* restrict _src, Entity* restrict target) {
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
static inline void ff_draw_debug_overlay(Field* restrict field, const Entity* restrict entity) {
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
static void ff_set_checkbox(Entity* restrict entity, [[maybe_unused]] int x, [[maybe_unused]] int y) {
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
        const float inverse = ext->inverse ? -1.0f : 1.0f;
        const auto step = ext->step[CP_COARSE] * inverse;
        value = step * roundf((float)dy * v) + ext->memory[CP_COARSE];
    }
    else {
        const int dx = field->memory[SP_CURSOR_PRIOR].x - x;
        const float inverse = ext->inverse ? -1.0f : 1.0f;
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
static void ff_scroll_slider(Entity* restrict entity, [[maybe_unused]] int x, int y) {
    auto fine = &entity->value[CP_FINE];
    auto coarse = &entity->value[CP_COARSE];
    auto ext = (Slider*)entity->extension;
    const float inverse = ext->inverse ? -1.0f : 1.0f;
    const auto df = (float)y * ext->step[CP_FINE] * inverse;

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
        const float inverse = ext->inverse ? -1.0f : 1.0f;
        const auto step = ext->step[CP_COARSE] * inverse;
        value = step * roundf((float)dy * v) + ext->memory[CP_COARSE];
    }
    else {
        const int dx = field->memory[SP_CURSOR_PRIOR].x - x;
        const float inverse = ext->inverse ? -1.0f : 1.0f;
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
        const float inverse = ext->inverse ? -1.0f : 1.0f;
        const auto step = ext->step[CP_FINE] * inverse;
        value = step * dy + *coarse;
    }
    else {
        const int dx = y;
        const float inverse = ext->inverse ? -1.0f : 1.0f;
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
    const auto range = ext->range[1] - ext->range[0];
    Sprite* spr = ext->sprite;
    const int f = ((entity->value[CP_COARSE] - ext->range[0]) / range) * spr->nframes;
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
    const int dy = roundf((field->memory[SP_CURSOR_PRIOR].y - y)/ext->step[CP_COARSE]);
    const int dx = roundf((x - field->memory[SP_CURSOR_PRIOR].x)/ext->step[CP_COARSE]);

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
    const int dy = roundf((y - field->memory[SP_CURSOR_PRIOR].y)/ext->step[CP_COARSE]);
    const int dx = roundf((field->memory[SP_CURSOR_PRIOR].x - x)/ext->step[CP_COARSE]);

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
static void ff_set_button(Entity* restrict entity, [[maybe_unused]] int x, [[maybe_unused]] int y) {
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
static void ff_release_button(Entity* restrict entity, [[maybe_unused]] int x, [[maybe_unused]] int y) {
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
static void ff_set_socket(Entity* restrict entity, [[maybe_unused]] int x, [[maybe_unused]] int y) {
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
    // Continuous-space centre — pixel i spans [i, i+1), so the geometric
    // centre of [bounds.l, bounds.r) is their midpoint. This gives the AA
    // circle sub-pixel accuracy even when width is odd.
    const float cx = ((float)entity->bounds.l + (float)entity->bounds.r) * 0.5f;
    const float cy = ((float)entity->bounds.t + (float)entity->bounds.b) * 0.5f;
    const float r_outer = (float)entity->width * 0.5f;
    const float r_inner = (float)entity->width * 0.25f;
    const auto color = entity->core_type == F_CT_OUTPUT ? SOCKET_OUT : SOCKET_IN;

    ff_draw_sdf_circle_fill(field->layer[FG], cx, cy, r_outer, color);
    ff_draw_sdf_circle_fill(field->layer[FG], cx, cy, r_inner, BUTTONS);
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
[[nodiscard]] static inline point ff_interpolate_bezier(point a, point b, point c, point d, float t) [[reproducible]]
{
    const float ti = 1.0f - t;
    const float tsq = t * t;
    const float isq = ti * ti;

    const float fa = isq * ti;
    const float fb = 3.0f * t * isq;
    const float fc = 3.0f * tsq * ti;
    const float fd = tsq * t;

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
static void ff_drag_socket(Entity* restrict entity, int x, int y)
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

    const float xo = (float)entity->bounds.l + 0.5f * (float)entity->width;
    const float yo = (float)entity->bounds.t + 0.5f * (float)entity->height;

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

    const auto uva = ff_screen_to_uv(a.x, a.y, o->width, o->height);
    const auto uvb = ff_screen_to_uv(b.x, b.y, o->width, o->height);
    const auto uvc = ff_screen_to_uv(c.x, c.y, o->width, o->height);
    const auto uvd = ff_screen_to_uv(d.x, d.y, o->width, o->height);

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

    const float xo = (float)entity->bounds.l + 0.5f * (float)entity->width;
    const float yo = (float)entity->bounds.t + 0.5f * (float)entity->height;

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

    const auto uva = ff_screen_to_uv(a.x, a.y, o->width, o->height);
    const auto uvb = ff_screen_to_uv(b.x, b.y, o->width, o->height);
    const auto uvc = ff_screen_to_uv(c.x, c.y, o->width, o->height);
    const auto uvd = ff_screen_to_uv(d.x, d.y, o->width, o->height);

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
[[nodiscard]] inline static bool ff_connect(Entity* restrict source, Entity* restrict target) {
    auto ext_s = (Socket*)source->extension;
    auto ext_t = (Socket*)target->extension;

    if(source->core_type == F_CT_INPUT && target->core_type == F_CT_OUTPUT) {
        ext_s->input = ext_t->output;
        if(ext_s->input_link) *ext_s->input_link = ext_t->output;
    }
    else if(source->core_type == F_CT_OUTPUT && target->core_type == F_CT_INPUT) {
        ext_t->input = ext_s->output;
        if(ext_t->input_link) *ext_t->input_link = ext_s->output;
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
        if(ext->input_link) *ext->input_link = nullptr;
    }
    else if(target && target->core_type == F_CT_INPUT) {
        auto target_ext = (Socket*)target->extension;
        target_ext->input = nullptr;
        if(target_ext->input_link) *target_ext->input_link = nullptr;
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

    if(target->type != ST_SOCKET) {
        ff_disconnect(entity);
        return;
    }

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
    auto field = entity->parent->parent;
    ext->frame = (Frame*)calloc(1, sizeof(Frame));
    /* Resized by ff_anchor_node with the window — policy_scale_init policy. */
    ff_frame_init_cap(ext->frame, entity->width, entity->height,
                      (size_t)entity->width * entity->height * field->policy_scale_init);
}

/** @brief Handle node press (prepares for potential drag).
 *  @param entity  Node entity.
 *  @param x       Mouse X (unused).
 *  @param y       Mouse Y (unused).
 */
static void ff_set_node(Entity* restrict entity, [[maybe_unused]] int x, [[maybe_unused]] int y) {
    entity->repaint = true;
}

/** @brief Check whether an entity's bounds overlap another node in the stencil.
 *  @param entity  Entity to test.
 *  @return @c true if any pixel in the entity's bounds belongs to a different node.
 */
[[nodiscard]] static inline bool ff_has_overlap(const Entity* restrict entity) {
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
static void ff_release_node(Entity* restrict entity, [[maybe_unused]] int x, [[maybe_unused]] int y)
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

/** Field Resize / Anchoring **************************************************************************************************/

/**
 * @brief Resize/move a node so its anchored edges sit on the field borders.
 * @details The node's screen rectangle is the bounds of its ST_NODE (frame)
 *          entity. Anchor semantics: an anchored edge is glued to its window
 *          border; the opposite edge keeps the node's size. A node therefore
 *          stretches only between two opposing anchors (e.g. a menu bar with
 *          LEFT|RIGHT|TOP spans the window but keeps its height) and
 *          translates when a single edge is anchored (a RIGHT-anchored panel
 *          sticks to the right border without growing). Axes without any
 *          anchor keep the node's position and size.
 *          The node's local layout is unchanged — only the frame grows or the
 *          whole node translates — so child entities keep their positions
 *          relative to the node and are translated by the move delta.
 *          Stencil layers (SN/SC) and the visible layers (NG/TL/FG) are
 *          repaired over the union of the old and new rectangles.
 * @param field  Owning field.
 * @param node   Node to re-anchor (visible, with an ST_NODE entity).
 */
static void ff_anchor_node(Field* restrict field, Node* restrict node) {
    /* The ST_NODE (frame) entity's bounds are the node's screen rectangle. */
    Entity* frame_entity = nullptr;
    for(uint32_t i = 0; i < node->entities; ++i) {
        if(node->at[i].type == ST_NODE) {
            frame_entity = &node->at[i];
            break;
        }
    }
    if(!frame_entity) return;

    const uint32_t a = node->anchors;
    const ltrb32u old = frame_entity->bounds;

    /* Anchored edge → glued to its border; opposite edge → keeps the node's
     * size. 64-bit intermediates keep a shrink from wrapping the unsigned
     * screen coordinates. */
    const uint32_t w = old.r - old.l;
    const uint32_t h = old.b - old.t;

    int64_t nl = old.l;
    int64_t nr = old.r;
    if (a & LEFT)       nl = field->bounds.l;
    else if (a & RIGHT) nl = (int64_t)field->bounds.r - (int64_t)w;
    if (a & RIGHT)      nr = field->bounds.r;
    else if (a & LEFT)  nr = (int64_t)field->bounds.l + (int64_t)w;

    int64_t nt = old.t;
    int64_t nb = old.b;
    if (a & TOP)         nt = field->bounds.t;
    else if (a & BOTTOM) nt = (int64_t)field->bounds.b - (int64_t)h;
    if (a & BOTTOM)      nb = field->bounds.b;
    else if (a & TOP)    nb = (int64_t)field->bounds.t + (int64_t)h;

    /* Anchored axes stay inside the field and non-empty (a node wider/taller
     * than the window is cropped to it). Free axes are not clamped — a shrink
     * may leave the node offscreen, which the bounds-checked draw/stencil
     * primitives handle safely. */
    if (a & (LEFT | RIGHT)) {
        if (nl < (int64_t)field->bounds.l) nl = field->bounds.l;
        if (nr > (int64_t)field->bounds.r) nr = field->bounds.r;
        if (nr <= nl) nr = nl + 1;
    }
    if (a & (TOP | BOTTOM)) {
        if (nt < (int64_t)field->bounds.t) nt = field->bounds.t;
        if (nb > (int64_t)field->bounds.b) nb = field->bounds.b;
        if (nb <= nt) nb = nt + 1;
    }

    const ltrb32u rect = {
        .l = (uint32_t)nl,
        .t = (uint32_t)nt,
        .r = (uint32_t)nr,
        .b = (uint32_t)nb,
    };
    if(rect.l == old.l && rect.t == old.t && rect.r == old.r && rect.b == old.b) return;

    const uint32_t new_w = rect.r - rect.l;
    const uint32_t new_h = rect.b - rect.t;
    const int32_t dx = (int32_t)rect.l - (int32_t)old.l;
    const int32_t dy = (int32_t)rect.t - (int32_t)old.t;
    const bool moved = dx != 0 || dy != 0;

    /* Union of old and new rectangles — cleared in every layer so no stale
     * pixels survive the resize/move. BG is fully redrawn via the canvas. */
    const ltrb32u uni = {
        .l = old.l < rect.l ? old.l : rect.l,
        .t = old.t < rect.t ? old.t : rect.t,
        .r = old.r > rect.r ? old.r : rect.r,
        .b = old.b > rect.b ? old.b : rect.b,
    };
    ff_draw_ltrb_f(field->layer[NG], &uni, 0x0);
    ff_draw_ltrb_f(field->layer[TL], &uni, 0x0);
    ff_draw_ltrb_f(field->layer[FG], &uni, 0x0);
    ff_draw_ltrb_f(field->layer[SC], &uni, 0x0);
    ff_draw_ltrb_f(field->layer[SN], &uni, 0x0);
    if(field->staging)
        ff_draw_ltrb_f(field->layer[ST], &uni, 0x0);

    /* Resize the node's local stencil buffer (entity indices are node-local and
     * unaffected by the resize) and the frame entity's own buffer. */
    ff_frame_resize(&node->buffer, new_w, new_h, field->policy_scale_num, field->policy_scale_den);
    auto frame_ext = (EntityNode*)frame_entity->extension;
    if(frame_ext && frame_ext->frame)
        ff_frame_resize(frame_ext->frame, new_w, new_h, field->policy_scale_num, field->policy_scale_den);

    node->width  = new_w;
    node->height = new_h;

    /* Stencils: node id over the new rectangle, entity ids re-copied from the
     * resized local buffer (covers both the moved and grown/shrunk cases). */
    ff_draw_ltrb_f(field->layer[SN], &rect, node->index);
    ff_frame_copy_at(field->layer[SC], &node->buffer, rect.l, rect.t);

    /* The frame entity takes the new rectangle; children follow the delta. */
    frame_entity->bounds  = rect;
    frame_entity->width   = new_w;
    frame_entity->height  = new_h;
    frame_entity->repaint = true;

    for(uint32_t i = 0; i < node->entities; ++i) {
        auto child = &node->at[i];
        if(child == frame_entity) continue;

        if(moved) {
            child->bounds.l = (uint32_t)((int64_t)child->bounds.l + dx);
            child->bounds.t = (uint32_t)((int64_t)child->bounds.t + dy);
            child->bounds.r = (uint32_t)((int64_t)child->bounds.r + dx);
            child->bounds.b = (uint32_t)((int64_t)child->bounds.b + dy);

            /* Sockets moved with the node — re-drag their cords. */
            if(child->type == ST_SOCKET) {
                auto child_ext = (Socket*)child->extension;
                if(child_ext->connected) {
                    auto target = child_ext->connection;
                    auto target_ext = (Socket*)target->extension;
                    if(child_ext->has_data)
                        ff_drag_cord(child, target->bounds.l + target->width / 2,
                                     target->bounds.t + target->height / 2);
                    else if(target_ext->has_data)
                        ff_drag_cord(target, child->bounds.l + child->width / 2,
                                     child->bounds.t + child->height / 2);
                }
            }
        }
        child->repaint = true;
    }

    /* SN changed — the canvas grid is drawn around node areas and must be
     * redrawn (same as ffPlaceNode / ff_release_node). */
    field->node[0].at[0].repaint = true;
    field->repaint = true;
}

/**
 * @brief Resize the field to new dimensions and re-anchor its nodes.
 * @details Resizes every render layer and the root canvas buffer (content is
 *          preserved, grown area cleared), then re-anchors every visible node
 *          with @ref Node::anchors != NONE to the new window borders. The
 *          hit-test stencils (SC/SN) are then rebuilt from the nodes' local
 *          stencil buffers, because ff_frame_resize crops on shrink — a node
 *          that was off-screen loses its stencil IDs and ff_draw_scene never
 *          re-stamps them. Cord splines and oscillograph trace points are
 *          stored in window-normalised UVs, so they are re-dragged / rescaled
 *          into the new window space to keep their screen positions. All
 *          entities are marked for repaint and every layer is fully dirty, so
 *          the backend re-uploads it. Resize is a rare event — the reallocs
 *          here are not on the per-frame path.
 * @param field  Field to resize.
 * @param w      New width in pixels (0 is ignored).
 * @param h      New height in pixels (0 is ignored).
 */
void ff_resize_field(Field* restrict field, uint32_t w, uint32_t h) {
    if(w == 0 || h == 0) return;

    const uint32_t old_w = field->width;
    const uint32_t old_h = field->height;

    field->width     = w;
    field->height    = h;
    field->bounds.r  = field->bounds.l + w;
    field->bounds.b  = field->bounds.t + h;

    for(uint32_t i = 0; i < CC; ++i)
        ff_frame_resize(field->layer[i], w, h, field->policy_scale_num, field->policy_scale_den);
    ff_frame_resize(&field->node[0].buffer, w, h, field->policy_scale_num, field->policy_scale_den);

    /* Node 0 is the root canvas — never anchored. */
    for(uint32_t n = 1; n < field->nodes; ++n) {
        if(!field->node[n].visible) continue;
        if(field->node[n].anchors == NONE) continue;
        ff_anchor_node(field, &field->node[n]);
    }

    /* Rebuild the hit-test stencils. SC/SN are a pure function of the
     * visible nodes: SN is the ST_NODE frame rect stamped with the node
     * index, SC is the node-local entity stencil (node->buffer) copied at
     * the frame position — the same stamping ffPlaceNode/ff_anchor_node do.
     * Node 0 is the canvas and is not stamped: SC/SN value 0 means "no
     * entity/node" and hit-tests resolve it to the root canvas. */
    ff_frame_clr(field->layer[SC]);
    ff_frame_clr(field->layer[SN]);
    for(uint32_t n = 1; n < field->nodes; ++n) {
        if(!field->node[n].visible) continue;

        /* The ST_NODE (frame) entity's bounds are the node's screen rect. */
        Entity* frame_entity = nullptr;
        for(uint32_t i = 0; i < field->node[n].entities; ++i) {
            if(field->node[n].at[i].type == ST_NODE) {
                frame_entity = &field->node[n].at[i];
                break;
            }
        }
        if(!frame_entity) continue;

        ff_draw_ltrb_f(field->layer[SN], &frame_entity->bounds, n);
        ff_frame_copy_at(field->layer[SC], &field->node[n].buffer,
                        frame_entity->bounds.l, frame_entity->bounds.t);
    }

    /* Re-drag established cords. Splines are stored in window-normalised UVs
     * (ff_screen_to_uv at drag time), so after a resize they map to different
     * screen positions and detach from their sockets. Recompute from the
     * current socket centres with the new window size — the same re-drag
     * ff_anchor_node/ff_release_node do when a node moves. A connection owns
     * exactly one cord (the has_data end); re-drag from that end. */
    for(uint32_t n = 0; n < field->nodes; ++n) {
        if(!field->node[n].visible) continue;
        for(uint32_t i = 0; i < field->node[n].entities; ++i) {
            auto entity = &field->node[n].at[i];
            if(entity->type != ST_SOCKET) continue;
            auto ext = (Socket*)entity->extension;
            if(!ext->connected) continue;
            auto target = ext->connection;
            auto target_ext = (Socket*)target->extension;
            if(ext->has_data)
                ff_drag_cord(entity, target->bounds.l + target->width / 2,
                             target->bounds.t + target->height / 2);
            else if(target_ext->has_data)
                ff_drag_cord(target, entity->bounds.l + entity->width / 2,
                             entity->bounds.t + entity->height / 2);
        }
    }

    /* Rescale oscillograph ring buffers. Trace points are stored in
     * window-normalised UVs, fed per frame with the current window size
     * (see the stream feed in field_glfw.h), so stored history maps to
     * different screen positions after a resize. X is proportional
     * (screen = uv * W) but Y is flipped (screen = H * (1 - uv)), so the
     * rescale is multiplicative in X and affine in Y — the trace keeps its
     * screen position, and new samples are fed with the new size. */
    for(size_t s = 0; s < sizeof(field->stream) / sizeof(field->stream[0]); ++s) {
        if(!field->stream[s]) continue;
        auto ext = (Oscillograph*)field->stream[s]->extension;
        if(!ext->data || !ext->size) continue;
        const float kx = (float)old_w / (float)w;
        const float ky = (float)old_h / (float)h;
        auto buf = (float*)ext->data;
        for(uint32_t i = 0; i < ext->size * 2; i += 2) {
            buf[i]     *= kx;
            buf[i + 1]  = 1.0f - ky * (1.0f - buf[i + 1]);
        }
    }

    /* Everything redraws: canvas grid, node frames, and all entities. */
    for(uint32_t n = 0; n < field->nodes; ++n) {
        if(!field->node[n].visible) continue;
        for(uint32_t i = 0; i < field->node[n].entities; ++i)
            field->node[n].at[i].repaint = true;
    }
    field->repaint = true;
}

/** @brief Initialise a textbox entity (no-op — buffer is embedded in the struct).
 *  @param entity  Textbox entity (unused).
 */
static void ff_init_textbox([[maybe_unused]] Entity* restrict entity) {
    // Text buffer is embedded in the Textbox extension, nothing to allocate
    (void)entity;
}

/** @brief Handle textbox press (marks for repaint).
 *  @param entity  Textbox entity.
 *  @param x       Mouse X (unused).
 *  @param y       Mouse Y (unused).
 */
static void ff_set_textbox(Entity* restrict entity, [[maybe_unused]] int x, [[maybe_unused]] int y) {
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
static void ff_draw_graticule(Frame* restrict frame, uint32_t colour, uint32_t vdiv, uint32_t hdiv) {
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
    auto field = entity->parent->parent;
    ext->frame = (Frame*)malloc(sizeof(Frame));
    ff_frame_init_cap(ext->frame, entity->width, entity->height,
                      (size_t)entity->width * entity->height * field->policy_scale_init);
    ff_draw_graticule(ext->frame, BACKGROUND, 10, 8);

    ext->size = 32;
    ext->data = (float*)calloc(ext->size * 2, sizeof(float));
    ext->r_head = 0;
    ext->w_head = 0;

    entity->parent->parent->stream[0] = entity;
}

/** @brief Handle CRT press (marks for repaint).
 *  @param entity  CRT entity.
 *  @param x       Mouse X (unused).
 *  @param y       Mouse Y (unused).
 */
static void ff_set_crt(Entity* restrict entity, [[maybe_unused]] int x, [[maybe_unused]] int y) {
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
static inline void ff_init_none([[maybe_unused]] Entity* restrict entity) {}

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
static inline void ff_set_none([[maybe_unused]] Entity* restrict entity, [[maybe_unused]] int x, [[maybe_unused]] int y) {}

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
static inline void ff_drag_none([[maybe_unused]] Entity* restrict entity, [[maybe_unused]] int x, [[maybe_unused]] int y) {}

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
static inline void ff_scroll_none([[maybe_unused]] Entity* restrict entity, [[maybe_unused]] int x, [[maybe_unused]] int y) {}

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
static inline void ff_enter_none([[maybe_unused]] Entity* restrict entity, [[maybe_unused]] int x, [[maybe_unused]] int y) {}

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
static inline void ff_leave_none([[maybe_unused]] Entity* restrict entity, [[maybe_unused]] int x, [[maybe_unused]] int y) {}

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
static inline void ff_release_none([[maybe_unused]] Entity* restrict entity, [[maybe_unused]] int x, [[maybe_unused]] int y) {}

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
