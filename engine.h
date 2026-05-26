#ifndef _ENGINE_H_
#define _ENGINE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"

#include "stb_ds.h"

#define COMMONLIB_REMOVE_PREFIX
#include "commonlib.h"

// #define ENGINE_IMPLEMENTATION

// Pre-defined shaders

static Shader __outline_shader = {0};
static const char *__outline_vert_shader = 
	"#version 330\n"
	"\n"
	"in vec3 vertexPosition;\n"
	"in vec2 vertexTexCoord;\n"
	"in vec3 vertexNormal;\n"
	"in vec4 vertexColor;\n"
	"\n"
	"uniform mat4 mvp; // Provided by raylib\n"
	"\n"
	"out vec4 fragColor;\n"
	"out vec2 fragTexCoord;\n"
	"\n"
	"void main() {\n"
	"	fragTexCoord = vertexTexCoord;\n"
	"	fragColor = vertexColor;\n"
	"\n"
	"	gl_Position = mvp*vec4(vertexPosition, 1.0);\n"
	"}\n";

static const char *__outline_frag_shader = 
	"#version 330\n"
	"\n"
	"in vec2 fragTexCoord;\n"
	"in vec4 fragColor;\n"
	"// in vec4 fragPosition;\n"
	"\n"
	"// Here the texture0 is the ren_tex (temp)\n"
	"uniform sampler2D texture0; // Provided by raylib\n"
	"uniform vec4 colDiffuse; // Provided by raylib\n"
	"\n"
	"out vec4 finalColor;\n"
	"\n"
	"uniform vec2 textureSize;\n"
	"uniform float outlineWidth;\n"
	"uniform vec4 outlineColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
	"\n"
	"int neighbor_count(vec2 uv)\n"
	"{\n"
	"    vec2 texel = 1.0 / textureSize;\n"
	"    int count = 0;\n"
	"\n"
	"	vec2 l_uv = uv + vec2(-texel.x * outlineWidth, 0);\n"
	"	l_uv.x = clamp(l_uv.x, 0.0, 1.0);\n"
	"	vec2 r_uv = uv + vec2(texel.x * outlineWidth, 0);\n"
	"	r_uv.x = clamp(r_uv.x, 0.0, 1.0);\n"
	"	vec2 t_uv = uv + vec2(0, -texel.y * outlineWidth);\n"
	"	t_uv.y = clamp(t_uv.y, 0.0, 1.0);\n"
	"	vec2 b_uv = uv + vec2(0, texel.y * outlineWidth);\n"
	"	b_uv.y = clamp(b_uv.y, 0.0, 1.0);\n"
	"\n"
	"	vec2 tl_uv = uv + vec2(-texel.x * outlineWidth, -texel.y * outlineWidth);\n"
	"	tl_uv.x = clamp(tl_uv.x, 0.0, 1.0);\n"
	"	tl_uv.y = clamp(tl_uv.y, 0.0, 1.0);\n"
	"\n"
	"	vec2 tr_uv = uv + vec2(texel.x * outlineWidth, -texel.y * outlineWidth);\n"
	"	tr_uv.x = clamp(tr_uv.x, 0.0, 1.0);\n"
	"	tr_uv.y = clamp(tr_uv.y, 0.0, 1.0);\n"
	"\n"
	"	vec2 bl_uv = uv + vec2(-texel.x * outlineWidth, texel.y * outlineWidth);\n"
	"	bl_uv.x = clamp(bl_uv.x, 0.0, 1.0);\n"
	"	bl_uv.y = clamp(bl_uv.y, 0.0, 1.0);\n"
	"\n"
	"	vec2 br_uv = uv + vec2(texel.x * outlineWidth, texel.y * outlineWidth);\n"
	"	br_uv.x = clamp(br_uv.x, 0.0, 1.0);\n"
	"	br_uv.y = clamp(br_uv.y, 0.0, 1.0);\n"
	"\n"
	"    if (texture(texture0, l_uv).a > 0.0) count++;\n"
	"    if (texture(texture0, r_uv).a > 0.0) count++;\n"
	"    if (texture(texture0, t_uv).a > 0.0) count++;\n"
	"    if (texture(texture0, b_uv).a > 0.0) count++;\n"
	"	 if (texture(texture0, tl_uv).a > 0.0) count++;\n"
	"	 if (texture(texture0, tr_uv).a > 0.0) count++;\n"
	"	 if (texture(texture0, bl_uv).a > 0.0) count++;\n"
	"	 if (texture(texture0, br_uv).a > 0.0) count++;\n"
	"    return count;\n"
	"}\n"
	"\n"
	"void main() {\n"
	"	vec2 texCoord = fragTexCoord;\n"
	"    vec4 texelColor = texture(texture0, texCoord);\n"
	"\n"
	"	if (texelColor.a < 0.5 && neighbor_count(fragTexCoord) > 0) {\n"
	"		finalColor = outlineColor * fragColor;\n"
	"	} else {\n"
	"		finalColor = texelColor * colDiffuse * fragColor;\n"
	"	}\n"
	"}\n";

// Globals
static int __screen_width, __screen_height;
static int __render_width, __render_height;
static float __screen_scale;
static RenderTexture2D __ren_tex = {0};
static RenderTexture2D __temp_ren_tex = {0};
static int __outline_shader_texture_size_loc = -1;
static int __outline_shader_outline_width_loc = -1;
static int __outline_shader_outline_color_loc = -1;

#define TEXTURE_OUTLINE_WIDTH_MAX 2.0f

// NOTE: Cam
/// Cam is just a Raylib Camera3D + pitch, yaw, roll, etc for easier controls
typedef struct {
	Camera3D cam3d;
	float pitch, yaw, roll;
	float dist_to_target;
	Vector3 vel, acc;

	// Look 
	float horz_speed, vert_speed;
	float move_speed;

} Cam;

Vector3 cam_forward(Cam *cam);
void cam_control(Cam *cam, float delta);
void cam_first_person_update(Cam *cam);

// NOTE: Mouse
static bool __eng_ignore_mouse_input = false;
Vector2 get_mpos_scaled();
bool mouse_button_pressed(int btn);
bool mouse_button_down(int btn);
bool mouse_button_released(int btn);
bool mouse_button_pressed_unignored(int btn);
bool mouse_button_down_unignored(int btn);
bool mouse_button_released_unignored(int btn);
void ignore_mouse_input(bool ignore);

// NOTE: Keyboard
#define key_pressed_or_repeated(key) (IsKeyPressed(key) || IsKeyPressedRepeat(key))

// Vector helpers
Vector3 v3xx(float v);
Vector3 v3(float x, float y, float z);
#define v3_add Vector3Add
#define v3_scale Vector3Scale
Vector2 v2xx(float v);
Vector2 v2(float x, float y);
#define v2_normalize Vector2Normalize
#define v2_add Vector2Add
#define v2_sub Vector2Subtract
#define v2_mag Vector2Length
#define v2_mag2 Vector2LengthSqr
#define v2_scale Vector2Scale
#define v2_mul Vector2Multiply
float v2_radians(Vector2 v);
float v2_degrees(Vector2 v);
Vector2 v2_from_radians(float r);
Vector2 v2_from_degrees(float d);

// NOTE: Vector2i
typedef struct {
	int x, y;
} Vector2i;

Vector2i v2vi(Vector2 v);
Vector2i v2i(int x, int y);
Vector2i v2ixx(int x);
bool v2i_equal(Vector2i a, Vector2i b);

// NOTE: Sprite
typedef struct Sprite Sprite;

#define SPRITE_DEFAULT_TIME_PER_FRAME 0.1f // seconds

struct Sprite {
	Texture2D texture;
	Rectangle tex_rect;
    Vector2i tex_offset;
	Vector2 pos;
	float width, height;
	float rotation;
	Vector2 origin;
	Vector2 scale;
	bool vflip, hflip;
	size_t vframes, hframes;
	size_t vframe, hframe;
	bool looped;
	float time_per_frame; // in seconds
	float accumulated_time;
	Color tint;
};

bool init_sprite(Sprite* spr, Texture tex, size_t hframes, size_t vframes);
bool init_sprite_from_sheet(Sprite *spr, Texture tex, Vector2i offset, Vector2i size, size_t hframes, size_t vframes);
void update_sprite_tex_rect(Sprite *spr);
void set_sprite_hframe(Sprite* spr, size_t hframe);
void set_sprite_vframe(Sprite* spr, size_t vframe);
void center_sprite_origin(Sprite* spr);
void draw_sprite(Sprite* spr);
void draw_sprite_at(Sprite *spr, Vector2 pos);
void draw_sprite_outlined_at(Sprite *spr, Vector2 pos, float outline_width, Color outline_color, Color tint);
void animate_sprite_hframes(Sprite* spr, float delta);
void free_sprite(Sprite* spr);
void set_sprite_scale_scalar(Sprite *spr, float scl);

// NOTE: Timer and Alarm
typedef struct Timer Timer;
typedef struct Alarm Alarm;

struct Timer {
		float time;
};

void update_timer(Timer *t, float dt);

struct Alarm {
		Timer timer;
		float alarm_time;
		bool once;
		bool done;
};

bool on_alarm(Alarm *a, float dt);

// NOTE: TextBox
typedef struct Textbox Textbox;

struct Textbox {
		char *buff;
		size_t buff_size;
		int cursor;
		const char *name;
		Font font;
		Vector2 pos;
		int font_size;
		Color active_color;
		Color inactive_color;
		bool active;
		Vector2 size;
		bool ignoring_input;

		int activate_key;
		int deactivate_key;
		char ignore_char;
};

Textbox make_textbox(Font font, int fs, Color active_color, Color inactive_color, Vector2 pos, Vector2 size, size_t buff_size, const char *name, char ignore_char);
void free_textbox(Textbox *tbox);
bool update_textbox(Textbox *tbox);
bool input_to_textbox(Textbox *tbox);
void set_textbox_keys(Textbox *tbox, int activate, int deactivate);
Rectangle get_textbox_rect(Textbox *tbox);
void draw_textbox(Textbox *tbox);
// NOTE: UI
typedef struct UI UI;
typedef struct UI_Theme UI_Theme;
typedef struct UI_Layout UI_Layout;
typedef struct UI_Draw_element UI_Draw_element;

typedef enum {
	UI_LAYOUT_KIND_HORZ,
	UI_LAYOUT_KIND_VERT,
	UI_LAYOUT_KIND_COUNT
} UI_Layout_kind;

struct UI_Layout {
	UI_Layout_kind kind;
	Vector2 pos;
	Vector2 size;
	Vector2 padding;
};

typedef enum {
	UI_DRAW_ELEMENT_TYPE_RECT,
	UI_DRAW_ELEMENT_TYPE_BOX,
	UI_DRAW_ELEMENT_TYPE_SPRITE,
	UI_DRAW_ELEMENT_TYPE_SPRITE_FRAME,
	UI_DRAW_ELEMENT_TYPE_TEXT,
	UI_DRAW_ELEMENT_TYPE_LINE,
	UI_DRAW_ELEMENT_TYPE_TEXTBOX,
	UI_DRAW_ELEMENT_TYPE_COUNT,
} UI_Draw_element_type;

const char *UI_Draw_element_type_as_str(const UI_Draw_element_type t);

struct UI_Draw_element {
	UI_Draw_element_type type;
	Vector2 pos;
	Vector2 size;
	Color fill_color;
	Color out_color;
	Sprite* spr;
	int hframe, vframe;
	Font* font;
	cstr text;
	int font_size;
    float thick;
    Textbox *tbox;
};

typedef struct {
	Arena arena;
	UI_Draw_element* buff;
	size_t count;
} UI_Draw_element_stack;

UI_Draw_element_stack UI_Draw_element_stack_make(void);
void UI_Draw_element_stack_push(UI_Draw_element_stack* stack, UI_Draw_element val);
bool UI_Draw_element_stack_pop(UI_Draw_element_stack* stack, UI_Draw_element* popped);
void UI_Draw_element_stack_free(UI_Draw_element_stack* stack);

Vector2 UI_Layout_available_pos(UI_Layout* this);
void UI_Layout_push_widget(UI_Layout* this, Vector2 size);

struct UI_Theme {
	Color bg_color;
	Color titlebar_color;
	int titlebar_padding;
	float titlebar_height;
	int titlebar_font_size;
	Vector2 bg_padding;
    float titlebar_pad_bottom;
};

UI_Theme get_default_ui_theme(void);
void set_ui_theme_titlebar_font_size(UI_Theme* theme, int font_size);

struct UI {
	int active_id;
	int last_used_id;
#define LAYOUTS_CAP 256
	UI_Layout layouts[LAYOUTS_CAP];
	size_t layouts_count;
	Vector2 btn_padding;
	int text_input_width; // in characters, so depends on the font_size
	Font* font;
	Rectangle bg_rect;
    Rectangle ui_rect; // @NOTE Rect covering the ui components (without bg_padding, etc)
	Vector2 pos;
	Vector2 pos_offset;
	bool is_moving;
	Alarm text_input_cursor_blink_alarm;
	bool show_text_input_cursor;
	UI_Draw_element_stack draw_element_stack;
	cstr title;
	bool show;
    UI_Theme theme;
    Vector2* mpos_ptr;
    Vector2 scroll_offset;
    Textbox tbox;
};

UI UI_make(UI_Theme theme, Font* font, Vector2 pos, cstr title, Vector2* mpos_ptr);
void UI_push_layout(UI* this, UI_Layout layout);
UI_Layout UI_pop_layout(UI* this);
UI_Layout* UI_top_layout(UI* this);
void UI_begin_layout(UI* this, UI_Layout_kind kind);
void UI_end_layout(UI* this);
void UI_free(UI* this);

void UI_begin(UI* this, UI_Layout_kind kind);
bool UI_button(UI* this, cstr text, int font_size, Color color);
void UI_text(UI* this, cstr text, int font_size, Color color);
void UI_line(UI* this, float thick, Color color);
void UI_spacing(UI* this, float spacing);
void UI_sprite(UI* this, Sprite* spr);
bool UI_sprite_button(UI* this, Sprite* spr);
bool UI_sprite_button_frame(UI* this, Sprite* spr, int hframe, int vframe);
bool UI_textbox(UI* this, Textbox *tbox);
bool UI_dropdown(UI* this, int *selected, const char **items, size_t items_count, bool *dropdown_expanded, int font_size, Color color);
void UI_background(UI* this);
// @NOTE: We are defering drawing because we need to call UI funcs before any input handling for the frame is happened,
// if we want input ignoring. We just push draw info to a stack when the UI funcs are called and draw all of them at once on UI_draw().
// @NOTE: @IMPORTANT: IT IS A MUST TO CALL THIS BEFORE UI_end()!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void UI_draw(UI* this);
// @NOTE: Must be in called input handling for the frame. ***
void UI_end(UI* this);

// NOTE: Rectangle
bool rect_contains_point(Rectangle r1, Vector2 p);
bool rect_contains_rect(Rectangle r1, Rectangle r2);
bool rect_intersects_rect(Rectangle r1, Rectangle r2);
Vector2 rotate_point(Vector2 p, float angle);
void rectangle_corners(Rectangle rect, float rot, Vector2 corners[4]);
bool overlap_on_axis(Vector2 axis, Vector2 rect1[4], Vector2 rect2[4]);
bool rectangles_intersect(Rectangle rect1, float rect1_rot, Rectangle rect2, float rect2_rot);
// bool rect_resolve_rect_collision(Rectangle* rect1, const Rectangle rect2);
// void rect_get_3d_points(Rectangle rect, Vector3f* p0, Vector3f* p1, Vector3f* p2, Vector3f* p3);
// void rect_get_points(Rectangle rect, Vector2* p0, Vector2* p1, Vector2* p2, Vector2* p3);

// NOTE: Window
bool init_window(int screen_width, int screen_height, float scl, const char *title, int *width_out, int *height_out);
void close_window(void);

typedef enum {
	TEXT_ALIGN_H_LEFT = 0,
	TEXT_ALIGN_H_CENTER,
	TEXT_ALIGN_H_RIGHT,
	TEXT_ALIGN_H_COUNT,
} Text_align_h;

// NOTE: Start enum at 10 to not conflict with Text_align_h
typedef enum {
	TEXT_ALIGN_V_TOP = 10,
	TEXT_ALIGN_V_CENTER,
	TEXT_ALIGN_V_BOTTOM,
	TEXT_ALIGN_V_COUNT,
} Text_align_v;

// NOTE: Draw
void draw_tex_rect(Texture2D tex, Vector2 pos, Vector2 offset, Vector2 size);
void draw_texture_outlined(Texture2D texture, Vector2 pos, float outline_width, Color outline_color, Color tint);
void draw_texture_centered(Texture2D tex, Vector2 pos, Color tint);
void begin_frame(void);
void end_frame(void);
void draw_ren_tex(RenderTexture2D ren_tex);
void draw_text_aligned(Font font, const char *text, Vector2 pos, int font_size, const Text_align_v align_v, const Text_align_h align_h, Color color);
void draw_text_aligned_ex(Font font, const char *text, Vector2 pos, int font_size, const Text_align_v align_v, const Text_align_h align_h, float rotation, Color color);
void draw_text(Font font, const char *text, Vector2 pos, int font_size, Color color);
void draw_3d_axis(Vector3 origin);

// NOTE: Input
bool input_to_buff(char *buff, size_t buff_cap, int *cursor);
bool input_to_buff_ignored(char *buff, size_t buff_cap, int *cursor, char ignore, bool *ignoring);

// NOTE: Assets Manager
typedef struct {
	char *key;
	Sound value;
} Sound_KV;

typedef struct {
	char *key;
	Texture2D value;
} Texture_KV;

typedef struct {
	Texture_KV *texture_map;
	Sound_KV *sound_map;
} Asset_manager;

bool load_texture(Asset_manager *am, const char *filepath, Texture2D *tex_out);
bool load_texture_(Asset_manager *am, const char *filepath, Texture2D *tex_out, bool verbose);
bool load_texture_from_data(Asset_manager *am, const char *name, unsigned char *data, int data_size, Texture2D *tex_out);
bool load_texture_from_data_(Asset_manager *am, const char *name, unsigned char *data, int data_size, Texture2D *tex_out, bool verbose);
bool load_sound(Asset_manager *am, const char *filepath, Sound *s_out);
bool load_sound_(Asset_manager *am, const char *filepath, Sound *s_out, bool verbose);
void clean_asset_manager(Asset_manager *am);

// NOTE: 9-slice Box
typedef struct Nine_slice_box Nine_slice_box;

struct Nine_slice_box {
	Vector2 tl, tr, bl, br, t, r, b, l;
	int tile_size;
	Texture2D tex;
	Color fill_color;
};

Nine_slice_box load_nine_slice_box(Asset_manager *am, const char *texpath, int tile_size, Vector2 tl, Vector2 tr, Vector2 bl, Vector2 br, Vector2 t, Vector2 r, Vector2 b, Vector2 l, Color fill_color);
void draw_nine_slice_box(Nine_slice_box *nbox, Vector2 pos, Vector2i size);

// NOTE: Console
#define CONSOLE_LINE_BUFF_CAP (1024*1)

typedef struct Console Console;
typedef struct Console_line Console_line;
typedef struct Console_lines Console_lines;

struct Console_line {
	char buff[CONSOLE_LINE_BUFF_CAP];
    size_t count;
    Color color;
};

struct Console_lines {
	Console_line *items;
	size_t count;
	size_t capacity;
}; // @darr


typedef enum Console_flag Console_flag;

enum Console_flag {
		CONSOLE_FLAG_NONE,
		CONSOLE_FLAG_READLINE_USES_UNPREFIXED_LINES,
		CONSOLE_FLAG_COUNT,
};

struct Console {
	Console_lines history;
	Console_lines lines;
    Console_lines unprefixed_lines;
	int cursor; // offset in the line
	int line;	 // line number
    Font font;
	int font_size;
    int hist_lookup_idx; // idx for Ctrl+P and Ctrl+N
    const char *prefix;
    const char *prefix2;
    char prefix_symbol;
    int flags; // int so can have 32 flags
    bool prompting;
    bool expecting_values;
    void (*prompt_done_func)(Console *, void *);
    void *prompt_userdata;
    String_array expected_prompt_values;
    size_t selected_prompt_value_id;
    size_t prompt_line_id;
};

typedef struct {
    int *items;
    size_t count;
    size_t capacity;
} Ids; // @darr

Console make_console(int flags, Font font, int font_size);
void add_line_to_console_simple(Console *console, char *line, Color color, bool hist);
void add_line_to_console(Console *console, char *buff, size_t buff_size, Color color, bool histt);
void add_line_to_console_prefixed(Console *console, Arena *tmp_arena, char *buff, Color color, bool histt);
void add_character_to_console_line(Console *console, char ch, size_t line);
Console_line *get_console_line(Console *console, size_t line);
Console_line *get_console_history(Console *console, size_t line);
Console_line *get_or_create_console_line(Console *console, size_t line);
void clear_console_line(Console_line *cl);
void clear_current_console_line(Console *console);
char *get_current_console_line_buff(Console *console);
const char *get_current_console_line_without_first_word(Console *console);
String_array get_current_console_args(Console *console);
bool input_to_console(Console *console, char *ignore_characters, size_t ignore_characters_count);
float get_cursor_offset(Console *console, int font_size);
void draw_console(Console *console, Rectangle rect, Vector2 pad, Color fill_color, Color border_color, float alpha);
void console_prompt(Console *console, const char *prompt, String_array *expected_prompt_values);
Ids match_command(const char *command, const char **commands, size_t commands_count);

// NOTE: Macros
#define log_info_console(console, fmt, ...) do {\
				Console_line l = {\
						.color = WHITE,\
				};\
				snprintf(l.buff, CONSOLE_LINE_BUFF_CAP, "[INFO] "fmt, __VA_ARGS__);\
				darr_append(console.lines, l);\
		} while (0)

#define log_warning_console(console, fmt, ...) do {\
				Console_line l = {\
						.color = YELLOW,\
				};\
				snprintf(l.buff, CONSOLE_LINE_BUFF_CAP, "[WARNING] "fmt, __VA_ARGS__);\
				darr_append(console.lines, l);\
		} while (0)

#define log_error_console(console, fmt, ...) do {\
				Console_line l = {\
						.color = RED,\
				};\
				snprintf(l.buff, CONSOLE_LINE_BUFF_CAP, "[ERROR] "fmt, ##__VA_ARGS__);\
				darr_append(console.lines, l);\
		} while (0)

#ifdef DEBUG
#define log_debug_console(console, fmt, ...) do {\
				Console_line l = {\
						.color = YELLOW,\
				};\
				snprintf(l.buff, CONSOLE_LINE_BUFF_CAP, "[DEBUG] "fmt, __VA_ARGS__);\
				darr_append(console.lines, l);\
		} while (0)
#else
#define log_debug_console(...)
#endif

#endif // _ENGINE_H_

// IMPLEMENTATION ////////////////////////////////
#ifdef ENGINE_IMPLEMENTATION

#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

// NOTE: Cam
Vector3 cam_forward(Cam *cam) {
	float pitch = cam->pitch;
	float yaw = cam->yaw;
	Vector3 forward = {
		.x = cos(pitch) * sin(yaw),
		.y = sin(pitch),
		.z = cos(pitch) * cos(yaw),
	};

	forward = Vector3Normalize(forward);

	return forward;
}

void cam_control(Cam *cam, float delta) {
	if (IsKeyDown(KEY_LEFT)) {
		cam->yaw += cam->horz_speed * delta;
	}
	if (IsKeyDown(KEY_RIGHT)) {
		cam->yaw -= cam->horz_speed * delta;
	}
	if (IsKeyDown(KEY_UP)) {
		cam->pitch += cam->vert_speed * delta;
	}
	if (IsKeyDown(KEY_DOWN)) {
		cam->pitch -= cam->vert_speed * delta;
	}

	float angle = cam->pitch;

	float maxAngleUp = Vector3Angle(cam->cam3d.up, cam->cam3d.target);
	maxAngleUp -= 0.001f; // avoid numerical errors
	if (angle > maxAngleUp) angle = maxAngleUp;

	// Clamp view down
	float maxAngleDown = Vector3Angle(Vector3Negate(cam->cam3d.up), cam->cam3d.target);
	maxAngleDown *= -1.0f; // downwards angle is negative
	maxAngleDown += 0.001f; // avoid numerical errors
	if (angle < maxAngleDown) angle = maxAngleDown;

	cam->pitch = angle;

	// Strafe
	if (IsKeyDown(KEY_A)) {
		Vector3 forward = cam_forward(cam);
		Vector3 left = Vector3Normalize(Vector3CrossProduct(cam->cam3d.up, forward));

		cam->cam3d.position = Vector3Add(cam->cam3d.position, Vector3Scale(left, cam->move_speed * delta));
	}
	if (IsKeyDown(KEY_D)) {
		Vector3 forward = cam_forward(cam);
		Vector3 left = Vector3Normalize(Vector3CrossProduct(cam->cam3d.up, forward));

		cam->cam3d.position = Vector3Add(cam->cam3d.position, Vector3Scale(left, -cam->move_speed * delta));
	}

	// Forward/Backward
	if (!IsKeyDown(KEY_LEFT_ALT) && IsKeyDown(KEY_W)) {
		Vector3 forward = cam_forward(cam);
		cam->cam3d.position = Vector3Add(cam->cam3d.position, Vector3Scale(forward, cam->move_speed * delta));
	}

	if (IsKeyDown(KEY_S)) {
		Vector3 forward = cam_forward(cam);
		cam->cam3d.position = Vector3Add(cam->cam3d.position, Vector3Scale(forward, -cam->move_speed * delta));
	}

	// Up/Down
	if (IsKeyDown(KEY_E)) {
		cam->cam3d.position.y += cam->move_speed * delta;
	}

	if (IsKeyDown(KEY_Q)) {
		cam->cam3d.position.y -= cam->move_speed * delta;
	}

	// Roll
	if (IsKeyDown(KEY_T)) {
		cam->roll += delta;
	}

	if (IsKeyDown(KEY_R)) {
		cam->roll -= delta;
	}
}

void cam_first_person_update(Cam *cam) {
	Vector3 forward = cam_forward(cam);

	cam->cam3d.target.x = cam->cam3d.position.x + forward.x * cam->dist_to_target;
	cam->cam3d.target.y = cam->cam3d.position.y + forward.y * cam->dist_to_target;
	cam->cam3d.target.z = cam->cam3d.position.z + forward.z * cam->dist_to_target;


	Vector3 world_up = v3(0, 1, 0);

	float cr = cosf(cam->roll);
	float sr = sinf(cam->roll);

	Vector3 up = {
		.x = world_up.x * cr + (forward.y * world_up.z - forward.z * world_up.y) * sr,
		.y = world_up.y * cr + (forward.z * world_up.x - forward.x * world_up.z) * sr,
		.z = world_up.z * cr + (forward.x * world_up.y - forward.y * world_up.x) * sr,
	};

	cam->cam3d.up = Vector3Normalize(up);

}

// NOTE: Mouse
Vector2 get_mpos_scaled() {
	Vector2 m = GetMousePosition();
	m.x *= __screen_scale;
	m.y *= __screen_scale;
	return m;
}

bool mouse_button_pressed(int btn) {
    if (__eng_ignore_mouse_input) return false;
    return IsMouseButtonPressed(btn);
}

bool mouse_button_down(int btn) {
    if (__eng_ignore_mouse_input) return false;
    return IsMouseButtonDown(btn);
}

bool mouse_button_released(int btn) {
    if (__eng_ignore_mouse_input) return false;
    return IsMouseButtonReleased(btn);
}

bool mouse_button_pressed_unignored(int btn) {
    return IsMouseButtonPressed(btn);
}

bool mouse_button_down_unignored(int btn) {
    return IsMouseButtonDown(btn);
}

bool mouse_button_released_unignored(int btn) {
    return IsMouseButtonReleased(btn);
}

void ignore_mouse_input(bool ignore) {
    __eng_ignore_mouse_input = ignore;
}

// Vector helpers
Vector3 v3xx(float v) { return CLITERAL(Vector3) { v, v, v }; }
Vector3 v3(float x, float y, float z) { return CLITERAL(Vector3) { x, y, z }; } 
Vector2 v2xx(float v) { return CLITERAL(Vector2) { v, v }; }
Vector2 v2(float x, float y) { return CLITERAL(Vector2) { x, y }; }
Vector2 v2_from_radians(float r) {
		Vector2 v = {0};

		v.x = cosf(r);
		v.y = sinf(r);

		return v;
}

Vector2 v2_from_degrees(float d) {
	return v2_from_radians(DEG2RAD*d);
}

float v2_radians(Vector2 v) {
		float angle = atan2f(v.y, v.x);
		return angle < 0 ? angle + (2*PI) : angle;	// Ensure the angle is positive
}

float v2_degrees(Vector2 v) {
	return v2_radians(v) * RAD2DEG;
}

// Vector2i
Vector2i v2vi(Vector2 v) { return CLITERAL(Vector2i) { (int)v.x, (int)v.y }; }

Vector2i v2i(int x, int y) { return (Vector2i) { .x = x, .y = y }; }
Vector2i v2ixx(int x)      { return v2i(x, x); }

bool v2i_equal(Vector2i a, Vector2i b) {
	return a.x == b.x && a.y == b.y;
}

// NOTE: 9-slice Box
Nine_slice_box load_nine_slice_box(Asset_manager *am, const char *texpath, int tile_size, Vector2 tl, Vector2 tr, Vector2 bl, Vector2 br, Vector2 t, Vector2 r, Vector2 b, Vector2 l, Color fill_color) {
	Nine_slice_box res = {
		.tile_size = tile_size,
		.tl = tl,
		.tr = tr,
		.bl = bl,
		.br = br,
		.t = t,
		.r = r,
		.b = b,
		.l = l,
		.fill_color = fill_color,
	};

	ASSERT(tile_size > 0, "TILE SIZE HAS TO BE GREATER THAN 0!");

	if (!load_texture(am, texpath, &res.tex)) {
		log_error("Failed to load 9slicebox texture `%s`", texpath);
		return res;
	}

	return res;
}

void draw_nine_slice_box(Nine_slice_box *nbox, Vector2 pos, Vector2i size) {
	// Top Left
	draw_tex_rect(nbox->tex, pos, v2_scale(nbox->tl, nbox->tile_size), v2xx(nbox->tile_size));
	
	// Top Right
	draw_tex_rect(nbox->tex, v2(pos.x + (size.x*nbox->tile_size), pos.y), v2_scale(nbox->tr, nbox->tile_size), v2xx(nbox->tile_size));
	
	// Bottom Left
	draw_tex_rect(nbox->tex, v2(pos.x, pos.y + (size.y*nbox->tile_size)), v2_scale(nbox->bl, nbox->tile_size), v2xx(nbox->tile_size));

	// Bottom Right
	draw_tex_rect(nbox->tex, v2(pos.x + (size.x*nbox->tile_size), pos.y + (size.y*nbox->tile_size)), v2_scale(nbox->br, nbox->tile_size), v2xx(nbox->tile_size));

	// Top
	for (int i = 1; i < size.x; ++i) {
		draw_tex_rect(nbox->tex, v2(pos.x + (i*nbox->tile_size), pos.y), v2_scale(nbox->t, nbox->tile_size), v2xx(nbox->tile_size));
	}

	// Right
	for (int i = 1; i < size.y; ++i) {
		draw_tex_rect(nbox->tex, v2(pos.x + (size.x*nbox->tile_size), pos.y + (i*nbox->tile_size)), v2_scale(nbox->r, nbox->tile_size), v2xx(nbox->tile_size));
	}

	// Bottom
	for (int i = 1; i < size.x; ++i) {
		draw_tex_rect(nbox->tex, v2(pos.x + (i*nbox->tile_size), pos.y + (size.y*nbox->tile_size)), v2_scale(nbox->b, nbox->tile_size), v2xx(nbox->tile_size));
	}

	// Left
	for (int i = 1; i < size.y; ++i) {
		draw_tex_rect(nbox->tex, v2(pos.x, pos.y + (i*nbox->tile_size)), v2_scale(nbox->l, nbox->tile_size), v2xx(nbox->tile_size));
	}

	// Fill
	Rectangle fill = {
		.x = nbox->tile_size,
		.y = nbox->tile_size,
		.width  = (size.x)*nbox->tile_size,
		.height = (size.y)*nbox->tile_size,
	};

	DrawRectangleRec(fill, nbox->fill_color);
}

// NOTE: UI
// @TODO: make an UI array struct and do begin/end and draws on them, make the active UI item the last in the array
// so that way the active UI item has the input.

extern bool should_ignore_mouse_input;

static Vector2 get_ui_bg_rect_pos(UI* this) {
    return (Vector2) {
        .x = this->pos.x - this->theme.bg_padding.x,
        .y = this->pos.y + (this->theme.titlebar_height * (1.f+this->theme.titlebar_pad_bottom)) - this->theme.bg_padding.y,
    };
}

Vector2 UI_Layout_available_pos(UI_Layout* this) {
	switch (this->kind) {
	case UI_LAYOUT_KIND_HORZ: {
		return (Vector2) {
			.x = this->pos.x + this->size.x + this->padding.x,
			.y = this->pos.y,
		};
	} break;
	case UI_LAYOUT_KIND_VERT: {
		return (Vector2) {
			.x = this->pos.x,
			.y = this->pos.y + this->size.y + this->padding.y,
		};
	} break;
	case UI_LAYOUT_KIND_COUNT:
	default: ASSERT(0, "Unreachable");
	}
	ASSERT(0, "Unreachable");

	return (Vector2) {0.f, 0.f};
}

void UI_Layout_push_widget(UI_Layout* this, Vector2 size) {
	switch (this->kind) {
	case UI_LAYOUT_KIND_HORZ: {
		this->size.x += size.x + this->padding.x;
		this->size.y = fmaxf(this->size.y, size.y);
	} break;
	case UI_LAYOUT_KIND_VERT: {
		this->size.x = fmaxf(this->size.x, size.x);
		this->size.y += size.y + this->padding.y;
	} break;
	case UI_LAYOUT_KIND_COUNT:
	default: ASSERT(0, "Unreachable");
	}
}

static void push_ui_widget(UI* this, UI_Layout* layout, Vector2 size) {
	switch (layout->kind) {
	case UI_LAYOUT_KIND_HORZ: {
		this->ui_rect.width += size.x;
		this->ui_rect.height = fmaxf(this->ui_rect.height, size.y);
	} break;
	case UI_LAYOUT_KIND_VERT: {
		this->ui_rect.width = fmaxf(this->ui_rect.width, size.x);
		this->ui_rect.height += size.y;
	} break;
	case UI_LAYOUT_KIND_COUNT:
	default: ASSERT(0, "Unreachable");
	}
    UI_Layout_push_widget(layout, size);
}

UI_Theme get_default_ui_theme(void) {
    UI_Theme res = {0};
    res.bg_color = GetColor(0x585B70FF),
    res.titlebar_color = GetColor(0x45475AFF),
	res.bg_padding = (Vector2) {10.f, 10.f};
    res.titlebar_pad_bottom = 0.5f;

    res.titlebar_padding = 4.f;
    set_ui_theme_titlebar_font_size(&res, 12);
    return res;
}

void set_ui_theme_titlebar_font_size(UI_Theme* theme, int font_size) {
    theme->titlebar_font_size = font_size;
	theme->titlebar_height = (float)(theme->titlebar_font_size + (theme->titlebar_padding*2.f));
}

UI UI_make(UI_Theme theme, Font* font, Vector2 pos, cstr title, Vector2* mpos_ptr) {
	UI res = {0};
	res.active_id = -1;
	res.layouts_count = 0;
	res.pos = pos;
	res.font = font;
	res.btn_padding = (Vector2) {4.f, 4.f};
	res.text_input_width = 12;
	res.text_input_cursor_blink_alarm.alarm_time = 0.5f;
	res.show_text_input_cursor = true;
	res.draw_element_stack = UI_Draw_element_stack_make();
	res.title = title;
    res.show = true;
    res.theme = theme;
    res.mpos_ptr = mpos_ptr;
	return res;
}

void UI_push_layout(UI* this, UI_Layout layout) {
	ASSERT(this->layouts_count < LAYOUTS_CAP, "Layouts exceeded");
	this->layouts[this->layouts_count++] = layout;
}

UI_Layout UI_pop_layout(UI* this) {
	ASSERT(this->layouts_count > 0, "Layouts exceeded");
	return this->layouts[--this->layouts_count];
}

UI_Layout* UI_top_layout(UI* this) {
	if (this->layouts_count > 0)
		return &this->layouts[this->layouts_count - 1];
	return NULL;
}

void UI_begin_layout(UI* this, UI_Layout_kind kind) {
	UI_Layout* prev = UI_top_layout(this);
	if (prev == NULL) {
		log_error("This function must be used between 'begin' and 'end'!");
		return;
	}

	UI_Layout next = {0};
	next.kind = kind;
	next.pos = UI_Layout_available_pos(prev);
	next.size = (Vector2) {0.f, 0.f};
	UI_push_layout(this, next);
}

void UI_end_layout(UI* this) {
	UI_Layout child = UI_pop_layout(this);
	UI_Layout* parent = UI_top_layout(this);
	if (parent == NULL) {
		log_error("This function must be used between 'begin' and 'end'!");
		return;
	}
    push_ui_widget(this, parent, child.size);
	/* UI_Layout_push_widget(parent, child.size); */
}

const char *UI_Draw_element_type_as_str(const UI_Draw_element_type t) {
	switch (t) {
        case UI_DRAW_ELEMENT_TYPE_RECT: return "Rect";
        case UI_DRAW_ELEMENT_TYPE_BOX: return "Box";
        case UI_DRAW_ELEMENT_TYPE_SPRITE: return "Sprite";
        case UI_DRAW_ELEMENT_TYPE_SPRITE_FRAME: return "Frame";
        case UI_DRAW_ELEMENT_TYPE_TEXT: return "Text";
        case UI_DRAW_ELEMENT_TYPE_LINE: return "Line";
        case UI_DRAW_ELEMENT_TYPE_TEXTBOX: return "Text Box";
        case UI_DRAW_ELEMENT_TYPE_COUNT:
        default: ASSERT(false, "UNREACHABLE!");
    }
}

UI_Draw_element_stack UI_Draw_element_stack_make(void) {
	UI_Draw_element_stack res = {0};

	const size_t draw_element_count = 100;
	res.arena = arena_make(sizeof(UI_Draw_element)*(draw_element_count+1));
	res.buff = (UI_Draw_element*)arena_alloc(&res.arena, sizeof(UI_Draw_element)*draw_element_count);

	return res;
}

void UI_Draw_element_stack_push(UI_Draw_element_stack* stack, UI_Draw_element val) {
	stack->buff[stack->count++] = val;
}

bool UI_Draw_element_stack_pop(UI_Draw_element_stack* stack, UI_Draw_element* popped) {
	if (stack->count == 0) {
		return false;
	} else {
		*popped = stack->buff[--stack->count];
	}
	return true;
}

void UI_Draw_element_stack_free(UI_Draw_element_stack* stack) {
	arena_free(&stack->arena);
}

void UI_begin(UI* this, UI_Layout_kind kind) {
	UI_Layout layout = {0};
	layout.pos = Vector2Add(this->pos, (Vector2) {0.f, this->theme.titlebar_height*(1+this->theme.titlebar_pad_bottom)});
    layout.pos = Vector2Add(layout.pos, this->scroll_offset);
	layout.kind = kind;
	UI_push_layout(this, layout);

    this->ui_rect.width = 0.f;
    this->ui_rect.height = 0.f;
}

bool UI_button(UI* this, cstr text, int font_size, Color color) {
	int id = this->last_used_id++;
	UI_Layout* top = UI_top_layout(this);
	if (top == NULL) {
		log_error("This function must be used between 'begin' and 'end'!");
		return false;
	}

	const Vector2 pos = UI_Layout_available_pos(top);
	const Vector2 size = Vector2Add(MeasureTextEx(*this->font, text, font_size, 1.f), Vector2Scale(this->btn_padding, 2.f));
	const Rectangle rect = {
			.x = pos.x,
			.y = pos.y,
			.width = size.x,
			.height = size.y,
	};
	bool click = false;
	Vector2 mpos = *this->mpos_ptr;
	bool hovering = CheckCollisionPointRec(mpos, rect);
	if (this->active_id == id) {
		if (mouse_button_released_unignored(MOUSE_BUTTON_LEFT)) {
			this->active_id = -1;
			if (hovering) {
                click = true;
			}
		}
	} else {
		if (hovering && mouse_button_pressed_unignored(MOUSE_BUTTON_LEFT)) {
			this->active_id = id;
		}
	}

	float alpha = 0.4f;
	if (hovering) {
		alpha = 0.5f;
	}

	bool is_clicked = (hovering && mouse_button_down_unignored(MOUSE_BUTTON_LEFT));
	if (is_clicked) {
		alpha = 1.f;
	}

	UI_Draw_element_stack_push(&this->draw_element_stack, (UI_Draw_element) {
			.type = UI_DRAW_ELEMENT_TYPE_BOX,
			.pos =	(Vector2) { rect.x, rect.y },
			.size = (Vector2) { rect.width, rect.height },
			.fill_color = ColorAlpha(color, 0.f),
			.out_color = WHITE,
		});

	Vector2 draw_pos = Vector2Add(pos, this->btn_padding);
	if (is_clicked) {
		draw_pos = Vector2AddValue(draw_pos, 1);
	}

	UI_Draw_element_stack_push(&this->draw_element_stack, (UI_Draw_element) {
			.type = UI_DRAW_ELEMENT_TYPE_TEXT,
			.pos = draw_pos,
			.fill_color = WHITE,
			.out_color = WHITE,
			.spr = NULL,
			.font = this->font,
			.text = text,
			.font_size = font_size,
		});

	UI_Draw_element_stack_push(&this->draw_element_stack, (UI_Draw_element) {
			.type = UI_DRAW_ELEMENT_TYPE_RECT,
			.pos =	(Vector2) { rect.x, rect.y },
			.size = (Vector2) { rect.width, rect.height },
			.fill_color = ColorAlpha(color, alpha),
		});
    push_ui_widget(this, top, size);
	/* UI_Layout_push_widget(top, size); */

    if (!this->show) click = false;
    if (click) ignore_mouse_input(true);

	return click;
}

void UI_text(UI* this, cstr text, int font_size, Color color) {
	int id = this->last_used_id++;
	(void)id;
	UI_Layout* top = UI_top_layout(this);
	if (top == NULL) {
		log_error("This function must be used between 'begin' and 'end'!");
		return;
	}

	const Vector2 pos = UI_Layout_available_pos(top);
	const Vector2 size = Vector2Add(MeasureTextEx(*this->font, text, font_size, 1.f), Vector2Scale(this->btn_padding, 2.f));
	/* draw_text(ctx, this->font, text, pos, font_size, color); */
	UI_Draw_element_stack_push(&this->draw_element_stack, (UI_Draw_element) {
			.type = UI_DRAW_ELEMENT_TYPE_TEXT,
			.pos = pos,
			.fill_color = color,
			.out_color = color,
			.spr = NULL,
			.font = this->font,
			.text = text,
			.font_size = font_size,
		});

    push_ui_widget(this, top, size);
    /* UI_Layout_push_widget(top, size); */
}

void UI_line(UI* this, float thick, Color color) {
	int id = this->last_used_id++;
	(void)id;
	UI_Layout* top = UI_top_layout(this);
	if (top == NULL) {
		log_error("This function must be used between 'begin' and 'end'!");
		return;
	}

	Vector2 pos = UI_Layout_available_pos(top);
    pos.y += thick;
	const Vector2 size = v2(this->ui_rect.width, thick * 4);

	UI_Draw_element_stack_push(&this->draw_element_stack, (UI_Draw_element) {
			.type = UI_DRAW_ELEMENT_TYPE_LINE,
			.pos = pos,
			.fill_color = color,
			.out_color = color,
			.spr = NULL,
            .thick = thick,
		});

    push_ui_widget(this, top, size);
}

void UI_sprite(UI* this, Sprite* spr) {
	int id = this->last_used_id++;
	(void)id;
	UI_Layout* top = UI_top_layout(this);
	if (top == NULL) {
		log_error("This function must be used between 'begin' and 'end'!");
		return;
	}

	const Vector2 pos = UI_Layout_available_pos(top);
	const Vector2 size = (Vector2) { spr->tex_rect.width*spr->scale.x, spr->tex_rect.height*spr->scale.y };
	/* draw_sprite_at(ctx, spr, pos); */
	UI_Draw_element_stack_push(&this->draw_element_stack, (UI_Draw_element) {
			.type = UI_DRAW_ELEMENT_TYPE_SPRITE,
			.pos = pos,
			.fill_color = WHITE,
			.out_color = WHITE,
			.spr = spr,
			.font = NULL,
			.text = NULL,
			.font_size = 0,
		});

    push_ui_widget(this, top, size);
	/* UI_Layout_push_widget(top, size); */
}

bool UI_sprite_button(UI* this, Sprite* spr) {
	int id = this->last_used_id++;
	UI_Layout* top = UI_top_layout(this);
	if (top == NULL) {
		log_error("This function must be used between 'begin' and 'end'!");
		return false;
	}

	const Vector2 pos = UI_Layout_available_pos(top);
	const Vector2 size = (Vector2) { spr->tex_rect.width, spr->tex_rect.height };
	const Rectangle rect = {
			.x = pos.x,
			.y = pos.y,
			.width = size.x,
			.height = size.y,
	};
	bool click = false;
	Vector2 mpos = *this->mpos_ptr;
	bool hovering = CheckCollisionPointRec(mpos, rect);
	if (this->active_id == id) {
		if (mouse_button_released_unignored(MOUSE_BUTTON_LEFT)) {
			this->active_id = -1;
			if (hovering) {
	click = true;
			}
		}
	} else {
		if (hovering && mouse_button_pressed_unignored(MOUSE_BUTTON_LEFT)) {
			this->active_id = id;
		}
	}

	float alpha = 0.4f;
	if (hovering) {
		alpha = 0.5f;
	}

	bool is_clicked = (hovering && mouse_button_down_unignored(MOUSE_BUTTON_LEFT));
	if (is_clicked) {
		alpha = 1.f;
	}

	Color color = spr->tint;
	color.a = alpha;
	UI_Draw_element_stack_push(&this->draw_element_stack, (UI_Draw_element) {
			.type = UI_DRAW_ELEMENT_TYPE_SPRITE,
			.pos = pos,
			.fill_color = color,
			.out_color = color,
			.spr = spr,
			.font = NULL,
			.text = NULL,
			.font_size = 0,
		});


    push_ui_widget(this, top, size);
	/* UI_Layout_push_widget(top, size); */

	// @Cleanup: why are we ignoring mouse input unconditionally here????
	/* ignore_mouse_input(true); */

    if (!this->show) click = false;
    if (click) ignore_mouse_input(true);
	return click;
}

bool UI_sprite_button_frame(UI* this, Sprite* spr, int hframe, int vframe) {
	int id = this->last_used_id++;
	UI_Layout* top = UI_top_layout(this);
	if (top == NULL) {
		log_error("This function must be used between 'begin' and 'end'!");
		return false;
	}

	const Vector2 pos = UI_Layout_available_pos(top);
	const Vector2 size = (Vector2) { spr->tex_rect.width, spr->tex_rect.height };
	const Rectangle rect = {
			.x = pos.x,
			.y = pos.y,
			.width = size.x,
			.height = size.y,
	};
	bool click = false;
	Vector2 mpos = *this->mpos_ptr;
	bool hovering = CheckCollisionPointRec(mpos, rect);
	if (this->active_id == id) {
		if (mouse_button_released_unignored(MOUSE_BUTTON_LEFT)) {
			this->active_id = -1;
			if (hovering) {
	click = true;
			}
		}
	} else {
		if (hovering && mouse_button_pressed_unignored(MOUSE_BUTTON_LEFT)) {
			this->active_id = id;
		}
	}

	float alpha = 0.4f;
	if (hovering) {
		alpha = 0.5f;
	}

	bool is_clicked = (hovering && mouse_button_down_unignored(MOUSE_BUTTON_LEFT));
	if (is_clicked) {
		alpha = 1.f;
	}

	Color color = spr->tint;
	color.a = alpha;
	/* draw_sprite_at(ctx, spr, pos); */
	UI_Draw_element_stack_push(&this->draw_element_stack, (UI_Draw_element) {
			.type = UI_DRAW_ELEMENT_TYPE_SPRITE_FRAME,
			.pos = pos,
			.fill_color = color,
			.out_color = color,
			.spr = spr,
			.hframe = hframe,
			.vframe = vframe,
			.font = NULL,
			.text = NULL,
			.font_size = 0,
		});

    push_ui_widget(this, top, size);
	/* UI_Layout_push_widget(top, size); */

	// @Cleanup: why are we ignoring mouse input unconditionally here????
	/* ignore_mouse_input(true); */

    if (!this->show) click = false;
    if (click) ignore_mouse_input(true);
	return click;
}

void UI_spacing(UI* this, float spacing) {
	int id = this->last_used_id++;
	(void)id;
	UI_Layout* top = UI_top_layout(this);
	if (top == NULL) {
		log_error("This function must be used between 'begin' and 'end'!");
		return;
	}

	Vector2 size = {
		.x = spacing,
		.y = 0.f,
	};

	if (top->kind == UI_LAYOUT_KIND_VERT) {
		size.x = 0.f;
		size.y = spacing;
	}

    push_ui_widget(this, top, size);
	/* UI_Layout_push_widget(top, size); */
}

bool UI_textbox(UI* this, Textbox *tbox) {
	UI_Layout* top = UI_top_layout(this);
	if (top == NULL) {
		log_error("This function must be used between 'begin' and 'end'!");
		return false;
	}

	const Vector2 pos = UI_Layout_available_pos(top);
    const Vector2 size = tbox->size;
    tbox->pos = pos;


    Vector2 mpos = *this->mpos_ptr;
    Rectangle tbox_rect = get_textbox_rect(tbox);

    // Activate on click
    if (CheckCollisionPointRec(mpos, tbox_rect)) {
        if (mouse_button_pressed_unignored(MOUSE_BUTTON_LEFT)) tbox->active = !tbox->active;
    }

	UI_Draw_element_stack_push(&this->draw_element_stack, (UI_Draw_element) {
			.type = UI_DRAW_ELEMENT_TYPE_TEXTBOX,
            .tbox = tbox,
		});

    push_ui_widget(this, top, size);

    update_textbox(tbox);

    if (tbox->active) {
        return input_to_textbox(tbox);
    }

    return false;
}

bool UI_dropdown(UI* this, int *selected, const char **items, size_t items_count, bool *expanded, int font_size, Color color) {
	C_ASSERT(selected && expanded, "selected or expanded is NULL!");
	UI_Layout* top = UI_top_layout(this);
	if (top == NULL) {
		log_error("This function must be used between 'begin' and 'end'!");
		return -1;
	}

	bool click = false;
	Vector2 max_size = {0};
	Vector2 pos = UI_Layout_available_pos(top);
	if (*expanded) {
		for (int i = 0; i < items_count; ++i) {
			const char *item = items[i];
			Vector2 size = Vector2Add(MeasureTextEx(*this->font, item, font_size, 1.f), Vector2Scale(this->btn_padding, 2.f));

			Rectangle rect = {
				.x = pos.x,
				.y = pos.y,
				.width = size.x,
				.height = size.y,
			};

			Vector2 mpos = *this->mpos_ptr;
			bool hovering = CheckCollisionPointRec(mpos, rect);

			if (hovering && mouse_button_pressed_unignored(MOUSE_BUTTON_LEFT)) {
				click = true;
				*expanded = false;
				*selected = i;
			}
			color.a = 100;

			if (hovering) {
				color.a = 200;
			}
			if (click) {
				color.a = 255;
			}

			UI_Draw_element_stack_push(&this->draw_element_stack, (UI_Draw_element) {
				.type = UI_DRAW_ELEMENT_TYPE_BOX,
				.pos =	(Vector2) { rect.x, rect.y },
				.size = (Vector2) { rect.width, rect.height },
				.fill_color = ColorAlpha(color, 0.f),
				.out_color = WHITE,
			});

			UI_Draw_element_stack_push(&this->draw_element_stack, (UI_Draw_element) {
					.type = UI_DRAW_ELEMENT_TYPE_TEXT,
					.pos = v2_add(pos, this->btn_padding),
					.fill_color = color,
					.out_color = color,
					.spr = NULL,
					.font = this->font,
					.text = item,
					.font_size = font_size,
				});

			pos.y += size.y + 2.f; // NOTE: const padding

			if (size.x > max_size.x) max_size.x = size.x;
			max_size.y += size.y + 2.f;
		}
	} else {
		const char *item = items[*selected];
		Vector2 size = Vector2Add(MeasureTextEx(*this->font, item, font_size, 1.f), Vector2Scale(this->btn_padding, 2.f));
		Rectangle rect = {
			.x = pos.x,
			.y = pos.y,
			.width = size.x,
			.height = size.y,
		};

		Vector2 mpos = *this->mpos_ptr;
		bool hovering = CheckCollisionPointRec(mpos, rect);

		if (hovering && mouse_button_pressed_unignored(MOUSE_BUTTON_LEFT)) {
			click = true;
			*expanded = true;
		}
		
		if (hovering) {
			color.a = 200;
		}
		if (click) {
			color.a = 255;
		}

		UI_Draw_element_stack_push(&this->draw_element_stack, (UI_Draw_element) {
			.type = UI_DRAW_ELEMENT_TYPE_BOX,
			.pos =	(Vector2) { rect.x, rect.y },
			.size = (Vector2) { rect.width, rect.height },
			.fill_color = ColorAlpha(color, 0.f),
			.out_color = WHITE,
		});

		UI_Draw_element_stack_push(&this->draw_element_stack, (UI_Draw_element) {
				.type = UI_DRAW_ELEMENT_TYPE_TEXT,
				.pos = v2_add(pos, this->btn_padding),
				.fill_color = color,
				.out_color = color,
				.spr = NULL,
				.font = this->font,
				.text = item,
				.font_size = font_size,
			});

		max_size = size;
	}

	push_ui_widget(this, top, max_size);

	return click;
}

static void UI_titlebar(UI* this) {
	Vector2 rect_pos = Vector2Subtract(this->pos, (Vector2) {this->theme.bg_padding.x, 0.f});
	Vector2 rect_size = v2(this->bg_rect.width, 
                        fmaxf(this->theme.titlebar_height, this->theme.titlebar_font_size + (2*this->theme.titlebar_padding)));
	Rectangle titlebar = {
			.x = rect_pos.x,
			.y = rect_pos.y,
			.width = rect_size.x,
			.height = rect_size.y,
	};

	DrawRectangleRec(titlebar, this->theme.titlebar_color);
	/* DrawRectangleLinesEx(titlebar, 1.f, WHITE); */

	Vector2 title_pos = v2(titlebar.x + this->theme.titlebar_padding, titlebar.y + this->theme.titlebar_padding);
    /* Vector2 ui_size = get_ui_size(this); */
	DrawTextEx(*this->font, TextFormat("%s", this->title), title_pos, this->theme.titlebar_font_size, 1.f, WHITE);
	/* DrawTextEx(*this->font, TextFormat("%s %d, %d | %d, %d", this->title, (int)this->scroll_offset.x, (int)this->scroll_offset.y, (int)ui_size.x, (int)ui_size.y), title_pos, this->theme.titlebar_font_size, 1.f, WHITE); */
}

void UI_background(UI* this) {
	DrawRectangleRec(this->bg_rect, this->theme.bg_color);
	/* DrawRectangleLinesEx(this->bg_rect, 1.f, WHITE); */
    /* DrawRectangleLinesEx(this->ui_rect, 1.f, BLUE); */
}

void UI_draw(UI* this) {
    UI_titlebar(this); if (!this->show) {
        // @NOTE: Clean up draw element stack
        this->draw_element_stack.count = 0;
        return;
    }
	UI_background(this);

    BeginScissorMode(this->bg_rect.x, this->bg_rect.y, this->bg_rect.width, this->bg_rect.height);
	UI_Draw_element elm = {0};
	while (UI_Draw_element_stack_pop(&this->draw_element_stack, &elm)) {
		switch (elm.type) {
            case UI_DRAW_ELEMENT_TYPE_RECT: {
                DrawRectangleRec((Rectangle) {elm.pos.x, elm.pos.y, elm.size.x, elm.size.y}, elm.fill_color);
            } break;
            case UI_DRAW_ELEMENT_TYPE_BOX: {
                DrawRectangleRec((Rectangle) {elm.pos.x, elm.pos.y, elm.size.x, elm.size.y}, elm.fill_color);
                DrawRectangleLinesEx((Rectangle) {elm.pos.x, elm.pos.y, elm.size.x, elm.size.y}, 1.f, elm.out_color);
            } break;
            case UI_DRAW_ELEMENT_TYPE_SPRITE: {
                Color previous_tint = elm.spr->tint;
                elm.spr->tint = elm.fill_color;
                Vector2 prev_spr_pos = elm.spr->pos;
                elm.spr->pos = elm.pos;
                draw_sprite(elm.spr);
                elm.spr->pos = prev_spr_pos;
                elm.spr->tint = previous_tint;
            } break;
            case UI_DRAW_ELEMENT_TYPE_SPRITE_FRAME: {
                Color previous_tint = elm.spr->tint;
                int prev_hframe = elm.spr->hframe;
                int prev_vframe = elm.spr->vframe;
                set_sprite_hframe(elm.spr, elm.hframe);
                set_sprite_vframe(elm.spr, elm.vframe);
                elm.spr->tint = elm.fill_color; Vector2 prev_spr_pos = elm.spr->pos;
                elm.spr->pos = elm.pos;
                draw_sprite(elm.spr);
                elm.spr->pos = prev_spr_pos;
                elm.spr->tint = previous_tint;
                set_sprite_hframe(elm.spr, prev_hframe);
                set_sprite_vframe(elm.spr, prev_vframe);
            } break;
            case UI_DRAW_ELEMENT_TYPE_TEXT: {
                DrawTextEx(*elm.font, elm.text, elm.pos, elm.font_size, 1.f, elm.fill_color);
            } break;
            case UI_DRAW_ELEMENT_TYPE_LINE: {
                Vector2 pos2 = v2(this->ui_rect.x + this->ui_rect.width, elm.pos.y);
                DrawLineEx(elm.pos, pos2, elm.thick, elm.out_color);
            } break;
            case UI_DRAW_ELEMENT_TYPE_TEXTBOX: {
                ASSERT(elm.tbox, ".tbox must be set for UI_DRAW_ELEMENT_TYPE_TEXTBOX!!");
                draw_textbox(elm.tbox);
            } break;
            case UI_DRAW_ELEMENT_TYPE_COUNT:
            default: ASSERT(0, "Unreachable!");
		}
	}
    EndScissorMode();
    // @TEMP
    // DrawCircleV(this->pos, 16.f, RED);
}

void UI_end(UI* this) {
    // Calculate bg_rect
    /* Vector2 ui_size      = get_ui_size(this); */
    Vector2 pos          = get_ui_bg_rect_pos(this);
    this->bg_rect.x      = pos.x;
    this->bg_rect.y      = pos.y;
    this->bg_rect.width  = this->ui_rect.width + this->theme.bg_padding.x;
    this->bg_rect.height  = this->ui_rect.height + this->theme.bg_padding.y;

    this->ui_rect.x      = this->bg_rect.x + this->theme.bg_padding.x;
    this->ui_rect.y      = this->bg_rect.y + this->theme.bg_padding.y;
    this->ui_rect.width   -= this->theme.bg_padding.x;
    this->ui_rect.height  -= this->theme.bg_padding.y;
    /* this->ui_rect.width  = 300.f; */
    /* this->ui_rect.height = 125.f; */

	Vector2 title_pos  = this->pos;
	Vector2 title_size = (Vector2) {this->bg_rect.width, this->theme.titlebar_height};
	Rectangle titlebar = {
			.x = title_pos.x,
			.y = title_pos.y,
			.width = title_size.x,
			.height = title_size.y,
	};
	if (!mouse_button_down_unignored(MOUSE_BUTTON_LEFT)) {
		this->is_moving = false;
	}

	Vector2 mpos = *this->mpos_ptr;
	if (mouse_button_pressed_unignored(MOUSE_BUTTON_LEFT) &&
			CheckCollisionPointRec(mpos, titlebar)) {
		/* this->pos_offset = Vector2Subtract(Vector2Subtract(mpos, title_pos), (Vector2) {this->theme.bg_padding.x, 0.f}); */
		this->pos_offset = Vector2Subtract(mpos, title_pos);
		this->is_moving = true;
	}

	if (this->is_moving) {
        ignore_mouse_input(true);
        this->pos = Vector2Subtract(mpos, this->pos_offset);
	} else {
        if (mouse_button_pressed_unignored(MOUSE_BUTTON_MIDDLE) &&
			CheckCollisionPointRec(mpos, titlebar)) {
            this->show = !this->show;
        }
    }

	// eat mouse input if clicked on ui rect
    Vector2 size = { this->bg_rect.width, this->bg_rect.height };

     Rectangle rect = {
        pos.x,
        pos.y,
        size.x,
        size.y
     };

     if ((this->show) && (mouse_button_down_unignored(MOUSE_BUTTON_LEFT) || mouse_button_down_unignored(MOUSE_BUTTON_MIDDLE) || mouse_button_down_unignored(MOUSE_BUTTON_RIGHT))&&
         (CheckCollisionPointRec(mpos, rect) || CheckCollisionPointRec(mpos, titlebar))) {
         ignore_mouse_input(true);
     }

	this->last_used_id = 0;
	UI_pop_layout(this);
}

void UI_free(UI* this) {
	UI_Draw_element_stack_free(&this->draw_element_stack);
}

// Sprite
bool init_sprite(Sprite* spr, Texture2D texture, size_t hframes, size_t vframes) {
	spr->texture = texture;
	spr->hframes = hframes;
	spr->vframes = vframes;
	spr->pos = (Vector2) {0.f, 0.f};
	spr->width = (float)spr->texture.width;
	spr->height = (float)spr->texture.height;
	spr->scale = (Vector2) {1.f, 1.f};
	spr->tex_rect.width = spr->width / (float)spr->hframes;
	spr->tex_rect.height = spr->height / (float)spr->vframes;
	set_sprite_hframe(spr, hframes);
	set_sprite_vframe(spr, vframes);
	spr->tint = WHITE;

	spr->time_per_frame = SPRITE_DEFAULT_TIME_PER_FRAME;
	spr->accumulated_time = 0.f;
	return true;
}

bool init_sprite_from_sheet(Sprite *spr, Texture tex, Vector2i offset, Vector2i size, size_t hframes, size_t vframes) {
	spr->texture = tex;
	spr->hframes = hframes;
	spr->vframes = vframes;
    spr->tex_offset = offset;
	spr->pos = (Vector2) {0.f, 0.f};
	spr->width  = (float)size.x;
	spr->height = (float)size.y;
	spr->scale = (Vector2) {1.f, 1.f};
	spr->tex_rect.width  = spr->width;
	spr->tex_rect.height = spr->height;
	set_sprite_hframe(spr, hframes);
	set_sprite_vframe(spr, vframes);
	spr->tint = WHITE;

	spr->time_per_frame = SPRITE_DEFAULT_TIME_PER_FRAME;
	spr->accumulated_time = 0.f;
	return true;
}

void update_sprite_tex_rect(Sprite *spr) {
	spr->tex_rect = (Rectangle) {
		.x = spr->tex_offset.x + (spr->tex_rect.width * (float)spr->hframe),
		.y = spr->tex_offset.y + (spr->tex_rect.height * (float)spr->vframe),
		.width  = spr->tex_rect.width,
		.height = spr->tex_rect.height,
	};
}

void set_sprite_hframe(Sprite* spr, size_t hframe) {
	if (hframe > spr->hframes-1) hframe = 0;
	spr->hframe = hframe;
	update_sprite_tex_rect(spr);
}

void set_sprite_vframe(Sprite* spr, size_t vframe) {
	if (vframe > spr->vframes-1) vframe = 0;
	spr->vframe = vframe;
	update_sprite_tex_rect(spr);
}

void center_sprite_origin(Sprite* spr) {
	spr->origin.x = spr->tex_rect.width / 2.f;
	spr->origin.y = spr->tex_rect.height / 2.f;
}

void draw_sprite(Sprite* spr) {
		Rectangle dest = {
            .x = spr->pos.x,
            .y = spr->pos.y,
            .width	= spr->tex_rect.width * spr->scale.x,
            .height = spr->tex_rect.height * spr->scale.y,
		};
		Vector2 origin = CLITERAL(Vector2) {
            .x = spr->origin.x * spr->scale.x,
            .y = spr->origin.y * spr->scale.y,
		};
		DrawTexturePro(spr->texture, spr->tex_rect, dest, origin, spr->rotation, spr->tint);
		/* DrawTexture(spr->texture, spr->pos.x, spr->pos.y, spr->tint); */
}

void draw_sprite_at(Sprite *spr, Vector2 pos) {
	const Vector2 spr_pos = spr->pos;

	spr->pos = pos;

	draw_sprite(spr);

	spr->pos = spr_pos;
}

void draw_sprite_outlined_at(Sprite *spr, Vector2 pos, float outline_width, Color outline_color, Color tint) {
	ASSERT(IsShaderValid(__outline_shader), "Outline Shader should be loaded!");
	if (outline_width > TEXTURE_OUTLINE_WIDTH_MAX) {
		outline_width = TEXTURE_OUTLINE_WIDTH_MAX;
		log_info("Clamped outline_width down to %f", TEXTURE_OUTLINE_WIDTH_MAX);
	}

	BeginShaderMode(__outline_shader);

	BeginTextureMode(__temp_ren_tex);
	ClearBackground(BLANK);

	float texture_size[2] = { (float)__temp_ren_tex.texture.width, (float)__temp_ren_tex.texture.height };
	SetShaderValue(__outline_shader, __outline_shader_texture_size_loc, texture_size, SHADER_UNIFORM_VEC2);
	SetShaderValue(__outline_shader, __outline_shader_outline_width_loc, &outline_width, SHADER_UNIFORM_FLOAT);
	float outline_color_[4] = { 
		outline_color.r / 255.0,
		outline_color.g / 255.0,
		outline_color.b / 255.0,
		outline_color.a / 255.0,
	};
	SetShaderValue(__outline_shader, __outline_shader_outline_color_loc, &outline_color_, SHADER_UNIFORM_VEC4);

    const Color og_tint = spr->tint;
    spr->tint = tint;
    draw_sprite_at(spr, pos);
    spr->tint = og_tint;

	EndTextureMode();

	BeginTextureMode(__ren_tex);
		draw_ren_tex(__temp_ren_tex);
	EndTextureMode();

	EndShaderMode();
	BeginTextureMode(__ren_tex);
}

void animate_sprite_hframes(Sprite* spr, float delta) {
	spr->accumulated_time += delta;
	if (spr->accumulated_time >= spr->time_per_frame) {
		spr->accumulated_time -= spr->time_per_frame;
		if (spr->hframe+1 >= spr->hframes) {
			spr->looped = true;
		}
		set_sprite_hframe(spr, spr->hframe+1);
	}
}

void free_sprite(Sprite* spr) {
		(void)spr;
}

void set_sprite_scale_scalar(Sprite *spr, float scl) {
    spr->scale = v2xx(scl);
}

// NOTE: TextBox
Textbox make_textbox(Font font, int fs, Color active_color, Color inactive_color, Vector2 pos, Vector2 size, size_t buff_size, const char *name, char ignore_char) {
		Textbox tbox = {
				.buff = calloc(buff_size, sizeof(char)),
				.buff_size = buff_size,
				.name = name,
				.font = font,
				.pos = pos,
				.size = size,
				.font_size = fs,
				.active_color = active_color,
				.inactive_color = inactive_color,
				.ignoring_input = true,
				.ignore_char = ignore_char,
		};

		return tbox;
}

void free_textbox(Textbox *tbox) {
    if (!tbox) return;
    if (tbox->buff) free(tbox->buff);
}

bool update_textbox(Textbox *tbox) {
    if (tbox->active) {
        if ((IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(tbox->deactivate_key)) || IsKeyPressed(KEY_ESCAPE)) {
            tbox->active = false;
            tbox->ignoring_input = true;
        }
    } else {
        if (IsKeyPressed(tbox->activate_key)) {
            tbox->active = true;
        }
    }

    return tbox->active;
}

bool input_to_textbox(Textbox *tbox) {
    if (!tbox->active) return false;
    if (input_to_buff_ignored(tbox->buff, tbox->buff_size, &tbox->cursor, tbox->ignore_char, &tbox->ignoring_input)) {
        tbox->ignoring_input = true;
        return true;
    }
    return false;
}

void set_textbox_keys(Textbox *tbox, int activate, int deactivate) {
		tbox->activate_key = activate;
		tbox->deactivate_key = deactivate;
}

Rectangle get_textbox_rect(Textbox *tbox) {
    float buff_measured = MeasureTextEx(tbox->font, tbox->buff, tbox->font_size, 1.f).x;
    float name_measured = MeasureTextEx(tbox->font, tbox->name, tbox->font_size, 1.f).x;
    float measure_pad = 20.f;
    int buff_x = tbox->pos.x + name_measured;
    Rectangle rect = {
        .x = buff_x + measure_pad,
        .y = tbox->pos.y,
        .width = fmaxf(tbox->size.x, buff_measured*1.1),
        .height = tbox->size.y,
    };

    return rect;
}

void draw_textbox(Textbox *tbox) {
    Rectangle rect = get_textbox_rect(tbox);
    draw_text(tbox->font, tbox->name, tbox->pos, tbox->font_size, tbox->active ? tbox->active_color : tbox->inactive_color);
    draw_text(tbox->font, tbox->buff, v2(rect.x+2, tbox->pos.y), tbox->font_size, tbox->active ? tbox->active_color : tbox->inactive_color);
    DrawRectangleLinesEx(rect, 1, tbox->active ? tbox->active_color : tbox->inactive_color);
}

// Rectangle
bool rect_contains_point(Rectangle r1, Vector2 p) {
	return (p.x >= r1.x && p.x < r1.x + r1.width &&
		p.y >= r1.y && p.y < r1.y + r1.height);
}

bool rect_contains_rect(Rectangle r1, Rectangle r2) {
	return (rect_contains_point(r1, v2(r2.x, r2.y)) &&
		rect_contains_point(r1, (Vector2) {r2.x + r2.width,
							 r2.y + r2.height}));
}

bool rect_intersects_rect(Rectangle r1, Rectangle r2) {
	const float rect1_l = r1.x;
	const float rect1_r = r1.x+r1.width;
	const float rect1_t = r1.y;
	const float rect1_b = r1.y+r1.height;

	const float rect2_l = r2.x;
	const float rect2_r = r2.x+r2.width;
	const float rect2_t = r2.y;
	const float rect2_b = r2.y+r2.height;

	return (rect1_r >= rect2_l &&
		rect1_l <= rect2_r &&
		rect1_t <= rect2_b &&
		rect1_b >= rect2_t);
}

Vector2 rotate_point(Vector2 p, float angle) {
    float angle_rad = angle * (C_PI / 180.0);
    Vector2 rotated;
    rotated.x = p.x * cos(angle_rad) - p.y * sin(angle_rad);
    rotated.y = p.x * sin(angle_rad) + p.y * cos(angle_rad);
    return rotated;
}

void rectangle_corners(Rectangle rect, float rot, Vector2 corners[4]) {
    float hw = rect.width / 2.0;
    float hh = rect.height / 2.0;

    Vector2 local_corners[4] = {
        {-hw, -hh},
        {-hw, hh},
        {hw, hh},
        {hw, -hh}
    };

    for (int i = 0; i < 4; i++) {
        corners[i] = rotate_point(local_corners[i], rot);
        corners[i].x += rect.x;
        corners[i].y += rect.y;
    }
}

bool overlap_on_axis(Vector2 axis, Vector2 rect1[4], Vector2 rect2[4]) {
    float min1 = 1e9, max1 = -1e9;
    float min2 = 1e9, max2 = -1e9;

    for (int i = 0; i < 4; i++) {
        float projection1 = Vector2DotProduct(axis, rect1[i]);
        float projection2 = Vector2DotProduct(axis, rect2[i]);

        if (projection1 < min1) min1 = projection1;
        if (projection1 > max1) max1 = projection1;

        if (projection2 < min2) min2 = projection2;
        if (projection2 > max2) max2 = projection2;
    }

    return !(max1 < min2 || max2 < min1);
}

bool rectangles_intersect(Rectangle rect1, float rect1_rot, Rectangle rect2, float rect2_rot) {
    Vector2 corners1[4], corners2[4];

    rectangle_corners(rect1, rect1_rot, corners1);
    rectangle_corners(rect2, rect2_rot, corners2);

    Vector2 axes[8];

    // Get the axes from both rectangles
    for (int i = 0; i < 4; i++) {
        Vector2 edge;

        // Rectangle 1 edges
        edge.x = corners1[(i + 1) % 4].x - corners1[i].x;
        edge.y = corners1[(i + 1) % 4].y - corners1[i].y;
        axes[i].x = -edge.y; // Perpendicular
        axes[i].y = edge.x;  // Perpendicular
    }

    for (int i = 0; i < 4; i++) {
        Vector2 edge;

        // Rectangle 2 edges
        edge.x = corners2[(i + 1) % 4].x - corners2[i].x;
        edge.y = corners2[(i + 1) % 4].y - corners2[i].y;
        axes[i + 4].x = -edge.y; // Perpendicular
        axes[i + 4].y = edge.x;  // Perpendicular
    }

    // Check for overlap on each axis
    for (int i = 0; i < 8; i++) {
        if (!overlap_on_axis(axes[i], corners1, corners2)) {
            return 0; // Separating axis found, rectangles do not intersect
        }
    }

    return 1; // No separating axis found, rectangles intersect
}


bool rect_resolve_rect_collision(Rectangle* rect1, const Rectangle rect2) {
	const float rect1_l = rect1->x;
	const float rect1_r = rect1->x+rect1->width;
	const float rect1_t = rect1->y;
	const float rect1_b = rect1->y+rect1->height;

	const float rect2_l = rect2.x;
	const float rect2_r = rect2.x+rect2.width;
	const float rect2_t = rect2.y;
	const float rect2_b = rect2.y+rect2.height;

	// resolve collision only if it ever happens
	if (rect_intersects_rect(*rect1, rect2)) {
		Vector2 cb2_bot = {0.f, rect2_b};
		Vector2 cb1_top = {0.f, rect1_t};
		float d2_top = v2_mag2(v2_sub(cb1_top, cb2_bot));
		Vector2 cb2_left = {rect2_l, 0.f};
		Vector2 cb1_right = {rect1_r, 0.f};
		float d2_right = v2_mag2(v2_sub(cb1_right, cb2_left));
		Vector2 cb2_right = {rect2_r, 0.f};
		Vector2 cb1_left = {rect1_l, 0.f};
		float d2_left = v2_mag2(v2_sub(cb1_left, cb2_right));
		Vector2 cb2_top = {0.f, rect2_t};
		Vector2 cb1_bot = {0.f, rect1_b};
		float d2_bot = v2_mag2(v2_sub(cb1_bot, cb2_top));

		float min_d2 = fminf(d2_top, fminf(d2_left, fminf(d2_right, d2_bot)));

		if (min_d2 == d2_top) {
			rect1->y = rect2_b;
		} else if (min_d2 == d2_left) {
			rect1->x = rect2_r;
		} else if (min_d2 == d2_right) {
			rect1->x = rect2_l - rect1->width;
		} else if (min_d2 == d2_bot) {
			rect1->y = rect2_t - rect1->height;
		} else {
			ASSERT(0, "UNREACHABLE");
		}
		return true;
	}
	return false;
}

// void rect_get_3d_points(Rectangle rect, Vector3f* p0, Vector3f* p1, Vector3f* p2, Vector3f* p3) {
//	 Vector2 p0_ = v2_add(rect.pos, (Vector2) {0.f, 0.f});
//	 Vector2 p1_ = v2_add(rect.pos, (Vector2) {rect.size.x, 0.f});
//	 Vector2 p2_ = v2_add(rect.pos, (Vector2) {rect.size.x, rect.size.y});
//	 Vector2 p3_ = v2_add(rect.pos, (Vector2) {0.f, rect.size.y});
//
//	 *p0 = (Vector3f) {p0_.x, p0_.y, 0.f};
//	 *p1 = (Vector3f) {p1_.x, p1_.y, 0.f};
//	 *p2 = (Vector3f) {p2_.x, p2_.y, 0.f};
//	 *p3 = (Vector3f) {p3_.x, p3_.y, 0.f};
// }
//
// void rect_get_points(Rectangle rect, Vector2* p0, Vector2* p1, Vector2* p2, Vector2* p3) {
//	 *p0 = v2_add(rect.pos, (Vector2) {0.f, 0.f});
//	 *p1 = v2_add(rect.pos, (Vector2) {rect.size.x, 0.f});
//	 *p2 = v2_add(rect.pos, (Vector2) {rect.size.x, rect.size.y});
//	 *p3 = v2_add(rect.pos, (Vector2) {0.f, rect.size.y});
// }
//

// Setup
bool init_window(int screen_width, int screen_height, float scl, const char *title, int *width_out, int *height_out) {
	SetTraceLogLevel(LOG_NONE);
	InitWindow(screen_width, screen_height, title);

	int width = screen_width * scl;
	int height = screen_height * scl;

	*width_out = width;
	*height_out = height;

	__screen_width = screen_width;
	__screen_height = screen_height;

	__render_width = width;
	__render_height = height;

	__screen_scale = scl;

	log_info("Created Window with dimensions %dx%d", screen_width, screen_height);

	__ren_tex = LoadRenderTexture((int)(width), (int)(height));

	__temp_ren_tex = LoadRenderTexture((int)(width), (int)(height));

	SetTextureWrap(__temp_ren_tex.texture, TEXTURE_WRAP_CLAMP);

	if (!IsRenderTextureValid(__ren_tex)) {
		log_error("Failed to create RenderTexture2D!");
		return false;
	}
	log_info("Created RenderTexture2D with dimensions %dx%d (Scaled down by %.2f)", __ren_tex.texture.width, __ren_tex.texture.height, scl);


	// Load pre-defined shaders
	__outline_shader = LoadShaderFromMemory(__outline_vert_shader, __outline_frag_shader);

#ifndef ENGINE_NO_LOG
	if (!IsShaderValid(__outline_shader)) {
		log_error("Failed to load outline shader!");
	} else {
		log_debug("Successfully loaded outline shader!");
	}
#endif

	__outline_shader_texture_size_loc = GetShaderLocation(__outline_shader, "textureSize");
	__outline_shader_outline_width_loc = GetShaderLocation(__outline_shader, "outlineWidth");
	__outline_shader_outline_color_loc = GetShaderLocation(__outline_shader, "outlineColor");


	return true;
}

void close_window(void) {
	UnloadRenderTexture(__temp_ren_tex);
	UnloadRenderTexture(__ren_tex);
	CloseWindow();
}

// Draw
void draw_tex_rect(Texture2D tex, Vector2 pos, Vector2 offset, Vector2 size) {

	Rectangle src = {
		.x = offset.x,
		.y = offset.y,
		.width = size.x,
		.height = size.y,
	};

	Rectangle dst = {
		.x = pos.x,
		.y = pos.y,
		.width = size.x,
		.height = size.y,
	};
	DrawTexturePro(tex, src, dst, v2xx(0), 0, WHITE);
}

void draw_texture_outlined(Texture2D texture, Vector2 pos, float outline_width, Color outline_color, Color tint) {
	ASSERT(IsShaderValid(__outline_shader), "Outline Shader should be loaded!");

	if (outline_width > TEXTURE_OUTLINE_WIDTH_MAX) {
		outline_width = TEXTURE_OUTLINE_WIDTH_MAX;
		log_info("Clamped outline_width down to %f", TEXTURE_OUTLINE_WIDTH_MAX);
	}

	BeginShaderMode(__outline_shader);

	BeginTextureMode(__temp_ren_tex);
	ClearBackground(BLANK);

	float texture_size[2] = { (float)__temp_ren_tex.texture.width, (float)__temp_ren_tex.texture.height };
	SetShaderValue(__outline_shader, __outline_shader_texture_size_loc, texture_size, SHADER_UNIFORM_VEC2);
	SetShaderValue(__outline_shader, __outline_shader_outline_width_loc, &outline_width, SHADER_UNIFORM_FLOAT);

	float outline_color_[4] = { 
		outline_color.r / 255.0,
		outline_color.g / 255.0,
		outline_color.b / 255.0,
		outline_color.a / 255.0,
	};
	SetShaderValue(__outline_shader, __outline_shader_outline_color_loc, &outline_color_, SHADER_UNIFORM_VEC4);

	draw_texture_centered(texture, pos, tint);

	EndTextureMode();

	BeginTextureMode(__ren_tex);
		draw_ren_tex(__temp_ren_tex);
	EndTextureMode();

	EndShaderMode();
	BeginTextureMode(__ren_tex);
}

void draw_texture_centered(Texture2D tex, Vector2 pos, Color tint) {
	Rectangle src = {
		.x = 0,
		.y = 0,
		.width = tex.width,
		.height = tex.height,
	};

	Rectangle dst = {
		.x = pos.x,
		.y = pos.y,
		.width = tex.width,
		.height = tex.height,
	};
	DrawTexturePro(tex, src, dst, v2(tex.width * 0.5, tex.height * 0.5), 0.f, tint);
}
void begin_frame(void) {
	BeginDrawing();
	BeginTextureMode(__ren_tex);
}

void end_frame(void) {
	EndTextureMode();
	draw_ren_tex(__ren_tex);
	EndDrawing();
}

void draw_ren_tex(RenderTexture2D ren_tex) {
	const Rectangle src = {
		.x = 0,
		.y = 0,
		.width = ren_tex.texture.width,
		// NOTE: We flip the height because y-axis is flipped internally (in opengl land probably)
		.height = -ren_tex.texture.height,
	};

	const Rectangle dst = {
		.x = 0,
		.y = 0,
		.width	= __screen_width,
		.height = __screen_height,
	};
	DrawTexturePro(ren_tex.texture, src, dst, CLITERAL(Vector2) { 0.f, 0.f }, 0.f, WHITE);
}

void draw_text_aligned(Font font, const char *text, Vector2 pos, int font_size, const Text_align_v align_v, const Text_align_h align_h, Color color) {
		draw_text_aligned_ex(font, text, pos, font_size, align_v, align_h, 0.0, color);
}

void draw_text_aligned_ex(Font font, const char *text, Vector2 pos, int font_size, const Text_align_v align_v, const Text_align_h align_h, float rotation, Color color) {
	Vector2 origin = {0};
	// RLAPI Vector2 MeasureTextEx(Font font, const char *text, float fontSize, float spacing);		// Measure string size for Font
	float spacing = 2.f;
	Vector2 text_size = MeasureTextEx(font, text, font_size, spacing);

	switch (align_h) {
		case TEXT_ALIGN_H_LEFT: {
		} break;
		case TEXT_ALIGN_H_CENTER: {
			origin.x = text_size.x * 0.5f;
		} break;
		case TEXT_ALIGN_H_RIGHT: {
			origin.x = text_size.x;
		} break;
		case TEXT_ALIGN_H_COUNT: {
		} break;
		default: ASSERT(false, "UNREACHABLE");
	}

	switch (align_v) {
		case TEXT_ALIGN_V_TOP: {
		} break;
		case TEXT_ALIGN_V_CENTER: {
			origin.y = text_size.y * 0.5f;
		} break;
		case TEXT_ALIGN_V_BOTTOM: {
			origin.y = text_size.y;
		} break;
		case TEXT_ALIGN_V_COUNT: {
		} break;
		default: ASSERT(false, "UNREACHABLE");
	}

	DrawTextPro(font, text, pos, origin, rotation, font_size, spacing, color);
}

void draw_text(Font font, const char *text, Vector2 pos, int font_size, Color color) {
	draw_text_aligned(font, text, pos, font_size, TEXT_ALIGN_V_TOP, TEXT_ALIGN_H_LEFT, color);
}

void draw_3d_axis(Vector3 origin) {
	DrawRay((Ray) { origin, v3(1, 0, 0) }, RED);
	DrawRay((Ray) { origin, v3(0, 1, 0) }, GREEN);
	DrawRay((Ray) { origin, v3(0, 0, 1) }, BLUE);
}

// NOTE: Input
bool input_to_buff(char *buff, size_t buff_cap, int *cursor) {
		return input_to_buff_ignored(buff, buff_cap, cursor, 0, NULL);
}

bool input_to_buff_ignored(char *buff, size_t buff_cap, int *cursor, char ignore, bool *ignoring) {
		int ch = 0;

		if ((*cursor) < 0) (*cursor) = 0;
		if ((*cursor) > buff_cap-1) (*cursor) = buff_cap-1;

		do {
				// Backspace
				if (IsKeyPressed(KEY_BACKSPACE) ||
						IsKeyPressedRepeat(KEY_BACKSPACE)) {
						if (*cursor > 0)
								buff[--(*cursor)] = '\0';
				}

				// Enter
				if (IsKeyPressed(KEY_ENTER)) {
						return true;
				}

				ch = GetCharPressed();

				if (ignoring && *ignoring && ignore > 0 && ch == ignore) {
						*ignoring = false;
						continue;
				}

				if (ch > 0) {
						buff[(*cursor)++] = (char)ch;
				}

		} while (ch > 0);
		return false;

}

// Assets Manager
bool load_texture(Asset_manager *am, const char *filepath, Texture2D *tex_out) {
		return load_texture_(am, filepath, tex_out, false);
}


bool load_texture_(Asset_manager *am, const char *filepath, Texture2D *tex_out, bool verbose) {
	Texture_KV *tex_KV = shgetp_null(am->texture_map, (char *)filepath);

	if (tex_KV != NULL) {
        if (tex_out) {
            *tex_out = tex_KV->value;
        }
        if (verbose) {
            log_debug("Found '%s' at texture_map index [%zu]", filepath, shlenu(am->texture_map));
        }
	} else {
		Texture2D tex = LoadTexture(filepath);
		if (!IsTextureValid(tex)) return false;
        if (tex_out) {
            *tex_out = tex;
        }
		shput(am->texture_map, (char *)filepath, tex);
        if (verbose) {
            log_debug("Added '%s' to texture_map index [%zu]", filepath, shlenu(am->texture_map));
        }
	}

	return true;
}

bool load_texture_from_data(Asset_manager *am, const char *name, unsigned char *data, int data_size, Texture2D *tex_out) {
		return load_texture_from_data_(am, name, data, data_size, tex_out, false);
}

bool load_texture_from_data_(Asset_manager *am, const char *name, unsigned char *data, int data_size, Texture2D *tex_out, bool verbose) {
 	Texture_KV *tex_KV = shgetp_null(am->texture_map, (char *)name);

	if (tex_KV != NULL) {
    if (tex_out) {
      *tex_out = tex_KV->value;
    }
    if (verbose) {
      log_debug("Found '%s' at texture_map index [%zu]", name, shlenu(am->texture_map));
    }
	} else {
    Image img = LoadImageFromMemory(".png", data, data_size);
		Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
		if (!IsTextureValid(tex)) return false;
    if (tex_out) {
      *tex_out = tex;
    }
		shput(am->texture_map, (char *)name, tex);
    if (verbose) {
        log_debug("Added '%s' (from memory) to texture_map index [%zu]", name, shlenu(am->texture_map));
    }
	}

	return true;
 
}

bool load_sound(Asset_manager *am, const char *filepath, Sound *s_out) {
	return load_sound_(am, filepath, s_out, false);
}

bool load_sound_(Asset_manager *am, const char *filepath, Sound *s_out, bool verbose) {
	Sound_KV *sound_KV = shgetp_null(am->sound_map, (char *)filepath);

	if (sound_KV != NULL) {
		if (s_out)
			*s_out = sound_KV->value;
		if (verbose) {
			log_debug("Found '%s' at sound_map index [%zu]", filepath, shlenu(am->sound_map));
        }
	} else {
		Sound sound = LoadSound(filepath);
		if (!IsSoundValid(sound)) return false;
			if (s_out)
				*s_out = sound;
		shput(am->sound_map, (char *)filepath, sound);
			if (verbose) {
				log_debug("Added '%s' to sound_map index [%zu]", filepath, shlenu(am->sound_map));
            }
	}

	return true;
}

void clean_asset_manager(Asset_manager *am) {
	shlen(am->texture_map);
	for (int i = 0; i < shlen(am->texture_map); ++i) {
		Texture2D tex = am->texture_map[i].value;
		UnloadTexture(tex);
	}

#ifndef ENGINE_NO_LOG
	log_debug("Unloaded %d texture(s)!", count);
#endif // ENGINE_NO_LOG
}

// Console
Console make_console(int flags, Font font, int font_size) {
	Console c = {0};

	c.font = font;
	c.flags = flags;
	c.font_size = font_size;

	Console_line l = {0};
	darr_append(c.lines, l);
	darr_append(c.unprefixed_lines, l);

	return c;
}

void add_line_to_console_simple(Console *console, char *line, Color color, bool hist) {
		Console_line cl = {
				.count = strlen(line),
				.color = color,
		};
		memcpy(cl.buff, line, cl.count);
		darr_append(console->lines, cl);
		darr_append(console->unprefixed_lines, cl);

	if (hist) {
		darr_append(console->history, cl);
	}
		console->hist_lookup_idx = console->history.count-1;
}

void add_line_to_console(Console *console, char *buff, size_t buff_size, Color color, bool hist) {
		Console_line cl = { .count = buff_size, };
		memcpy(cl.buff, buff, buff_size);
		cl.color = color;
		darr_append(console->lines, cl);
		darr_append(console->unprefixed_lines, cl);

	if (hist) {
		darr_append(console->history, cl);
	}
		console->hist_lookup_idx = console->history.count-1;
}

void add_line_to_console_prefixed(Console *console, Arena *tmp_arena, char *buff, Color color, bool hist) {
		const char *prefixed = arena_alloc_str(*tmp_arena, "%s%s%c%s", console->prefix, console->prefix2, console->prefix_symbol, buff);
		size_t prefixed_len = strlen(prefixed);

		Console_line ucl = { .count = strlen(buff) };
		memcpy(ucl.buff, buff, ucl.count);
		darr_append(console->unprefixed_lines, ucl);

		Console_line cl = { .count = prefixed_len, };
		memcpy(cl.buff, prefixed, prefixed_len);
		cl.color = color;
		darr_append(console->lines, cl);

	if (hist) {
		darr_append(console->history, cl);
	}
		console->hist_lookup_idx = console->history.count-1;
}

void add_character_to_console_line(Console *console, char ch, size_t line) {
		Console_line *l = get_console_line(console, line);
		Console_line *ul = &console->unprefixed_lines.items[line];
		if (l == NULL) {
				return;
		}

		l->buff[l->count++] = ch;
		ul->buff[ul->count++] = ch;
}

Console_line *get_console_line(Console *console, size_t line) {
    if (line >= console->lines.count) {
        log_error("Outofbounds: %zu is out of bounds of lines.count (%zu)", line, console->lines.count);
        return NULL;
    }

    return &console->lines.items[line];
}

Console_line *get_console_history(Console *console, size_t line) {
		if (line >= console->history.count) {
				log_error("Outofbounds: %zu is out of bounds of history.count (%zu)", line, console->history.count);
				return NULL;
		}

		return &console->history.items[line];
}

Console_line *get_or_create_console_line(Console *console, size_t line) {
		if (console->lines.count < line+1) {
				Console_line new_console_line = {0};
				darr_append(console->lines, new_console_line);
		}
		return get_console_line(console, line);
}

void clear_console_line(Console_line *cl) {
    if (cl == NULL) {
        log_warning("Console line is NULL!!");
        return;
    }
    memset(cl->buff, 0, CONSOLE_LINE_BUFF_CAP);
}

void clear_current_console_line(Console *console) {
		Console_line *cl = get_or_create_console_line(console, console->line);
		clear_console_line(cl);
		console->cursor = 0;
}

char *get_current_console_line_buff(Console *console) {
		if (console == NULL) return NULL;

		if (console->line >= console->lines.count) {
				log_error("Outofbounds: %d is out of bounds of lines.count (%zu)", console->line, console->lines.count);
				return NULL;
		}

		return console->lines.items[console->line].buff;
}

const char *get_current_console_line_without_first_word(Console *console) {
    const char *buff = get_current_console_line_buff(console);

    while (!isspace(*buff) && *buff != '\0') { buff++; }

    if (*buff == '\0') return NULL;

    while (isspace(*buff)) { buff++; }

    return buff;
}

String_array get_current_console_args(Console *console) {
		String_array res = {0};

		const char *buff = get_current_console_line_buff(console);
		String_view sv = SV(buff);

		sv_trim(&sv);
		while (sv.count > 0) {
				sv_trim(&sv);
				String_view arg = {0};
				if (!sv_lpop_arg(&sv, &arg)) break;
				char *str = sv_to_cstr(arg);
				darr_append(res, str);

				// skip spaces between args
				sv_trim(&sv);
		}

		return res;
}

void readline_update(Console *console, Console_line *line) {
	Console_line *last_line = get_console_history(console, console->hist_lookup_idx);
	bool should_get_unprefixed_lines = GET_FLAG(console->flags, CONSOLE_FLAG_READLINE_USES_UNPREFIXED_LINES);
	if (should_get_unprefixed_lines) {
		if (console->hist_lookup_idx >= console->unprefixed_lines.count) {
			log_error("Outofbounds: %d is out of bounds of unprefixed_lines.count (%zu)", console->hist_lookup_idx, console->unprefixed_lines.count);
			return;
		}
		last_line = &console->unprefixed_lines.items[console->hist_lookup_idx];
	}
	if (last_line != NULL)	{
		memcpy(line->buff, last_line->buff, last_line->count);
		line->count = last_line->count;
		line->buff[line->count] = '\0';
		console->cursor = strlen(line->buff);
	}
}

bool input_to_console(Console *console, char *ignore_characters, size_t ignore_characters_count) {
	int ch = 0;
		Console_line *line = get_or_create_console_line(console, console->line);

		if (console->cursor < 0) console->cursor = 0;
		if (console->cursor > CONSOLE_LINE_BUFF_CAP-1) console->cursor = CONSOLE_LINE_BUFF_CAP-1;

	int chars_inputted = 0;

	do {
		ch = GetCharPressed();

		if (ch > 0) chars_inputted++;

				bool ignore = false;

				for (size_t i = 0; i < ignore_characters_count; ++i) {
						if (ch == ignore_characters[i]) {
								ignore = true;
								break;
						}
				}

				if (ignore) continue;

				if (IsKeyPressed(KEY_ENTER)) {
						if (console->prompting && console->expecting_values) {
								bool found = false;
								for (size_t i = 0; i < console->expected_prompt_values.count; ++i) {
										const char *expecting = console->expected_prompt_values.items[i]; 
										if (strcmp(line->buff, expecting) == 0) {
												console->selected_prompt_value_id = i;
												found = true;
												break;
										}
								}
								if (!found) {
										log_error_console((*console), "Prompt expects: ");
										for (int i = 0; i < console->expected_prompt_values.count; ++i) {
												log_error_console((*console), "		- %s", console->expected_prompt_values.items[i]);
										}
								}

								console->prompting = false;
								if (console->prompt_done_func)
										console->prompt_done_func(console, console->prompt_userdata);

								console->prompt_done_func = NULL;
								console->prompt_userdata = NULL;
								clear_current_console_line(console);

								darr_remove_unordered(console->lines, console->prompt_line_id);

								return false;
						}

			// if (chars_inputted <= 0) {
			// 	darr_delete(console->lines, Console_line, console->line);
			// }

						return true;
				}

				// readline functionality
				if (IsKeyDown(KEY_LEFT_CONTROL)) {
						// Prev_line
						if (IsKeyPressed(KEY_P)) {
								if (console->lines.count > 0) {
										if (console->hist_lookup_idx > 1) {
						console->hist_lookup_idx--;
					}
					// log_info("%d", console->hist_lookup_idx);
					readline_update(console, line);
								}
						}

						if (IsKeyPressed(KEY_N)) {
								if (console->lines.count > 0) {
										if (console->hist_lookup_idx < (int)(console->history.count)-1) {
						console->hist_lookup_idx++;
					}

					// log_info("%d (%s)", console->hist_lookup_idx, console->hist_lookup_idx < console->history.count-2 ? "true" : "false");
					readline_update(console, line);
								}
						}
				}

				if (IsKeyPressed(KEY_BACKSPACE) ||
						IsKeyPressedRepeat(KEY_BACKSPACE)) {
						if (console->cursor > 0) {
								line->buff[--console->cursor] = '\0';
						}
				}

				if (line->count > CONSOLE_LINE_BUFF_CAP) {
						log_error("Exhausted line buff!");
						exit(1);
				}

				if (ch > 0) {
						// log_debug("TYPED %c AT %d:%d", (char)ch, console->line, console->cursor);
						// log_debug("CODEPOINT %c: %fx%f", ch, codepoint_rec.width, codepoint_rec.height);
						line->buff[console->cursor++] = (char)ch;
				}

				
	} while (ch > 0);

	// if (chars_inputted <= 0) {
	// 	darr_delete(console->lines, Console_line, console->line);
	// }

		return false;
}

// float get_cursor_offset(Console *console, int font_size) {
//		 Font font = console->font;
//
//		 char buf[1024];
//
//		 char *text = get_current_console_line_buff(console);
//		 memcpy(buf, text, console->cursor);
//		 buf[console->cursor] = '\0';
//
//		 Vector2 size = MeasureTextEx(font, buf, font_size, 1.f);
//
//		 return size.x;
// }

float get_cursor_offset(Console *console, int font_size) {
		Font font = console->font;
		const char *text = get_current_console_line_buff(console);
		float scale = (float)font_size / (float)font.baseSize;
		float x = 0.0f;

		for (int i = 0; i < console->cursor && text[i] != '\0'; ) {
				int codepoint = text[i];
				// GlyphInfo glyph = GetGlyphInfo(font, codepoint); // pseudo: access font.glyphs[...] or use raylib helpers
				i += 1;
				Rectangle glyph_atlas_rec = GetGlyphAtlasRec(font, codepoint);
				x += glyph_atlas_rec.width * scale;

				// log_info("%c: %f, %f, %fx%f", codepoint, glyph_atlas_rec.x, glyph_atlas_rec.y, glyph_atlas_rec.width, glyph_atlas_rec.height);
		}


		return x;
}

void draw_console(Console *console, Rectangle rect, Vector2 pad, Color fill_color, Color border_color, float alpha) {
	Vector2 pos = {rect.x, rect.y + (rect.height - console->font_size)};
	pos = Vector2Add(pos, pad);
	DrawRectangleRec(rect, ColorAlpha(fill_color, alpha));
	DrawRectangleLinesEx(rect, 1.f, ColorAlpha(border_color, alpha));
	BeginScissorMode(rect.x, rect.y, rect.width, rect.height);

	for (size_t i = 0; i < console->lines.count; ++i) {
		Console_line *line = &console->lines.items[console->lines.count - i - 1];
		draw_text(GetFontDefault(), line->buff, pos, console->font_size, ColorAlpha(line->color, alpha));

		pos.y -= (pad.y + console->font_size);
	}

	EndScissorMode();

	Rectangle current_line_rect = {
		.x = rect.x,
		.y = rect.y + rect.height,
		.width = rect.width,
		.height = console->font_size,
	};

	DrawRectangleRec(current_line_rect, fill_color);
	DrawRectangleLinesEx(current_line_rect, 1.f, ColorAlpha(border_color, alpha));

	// @SPEED
	char actual_prefix[1024] = {0};
	
	snprintf(actual_prefix, 1024, "%s%s%c", console->prefix ? console->prefix : "", console->prefix2 ? console->prefix2 : "", console->prefix_symbol);

	draw_text(console->font, actual_prefix, v2(rect.x + 4.f, rect.y + rect.height), console->font_size, ColorAlpha(WHITE, alpha));
	float prefix_offset = MeasureTextEx(console->font, actual_prefix, console->font_size, 2.5f).x + 10.f;
	draw_text(console->font, get_current_console_line_buff(console), v2(rect.x + prefix_offset, rect.y + rect.height), console->font_size, ColorAlpha(WHITE, alpha));

	// Rectangle cursor_rec = {
	//		 .x = rect.x + get_cursor_offset(console, console->font_size),
	//		 .y = rect.y + rect.height,
	//		 .width = console->font_size,
	//		 .height = console->font_size,
	// };
	// DrawRectangleRec(cursor_rec, WHITE);

	// log_debug("console->cursor: %d", console->cursor);
}

void console_prompt(Console *console, const char *prompt, String_array *expected_prompt_values) {
		console->prompting = true;
		add_line_to_console_simple(console, (char *)prompt, GOLD, false);
		console->prompt_line_id = console->lines.count-1;
		console->expecting_values = expected_prompt_values != NULL;
		if (expected_prompt_values != NULL) {
				console->expected_prompt_values.count = 0;
				for (int i = 0; i < expected_prompt_values->count; ++i) {
						darr_append(console->expected_prompt_values, expected_prompt_values->items[i]);
				}
		}
}

Ids match_command(const char *command, const char **commands, size_t commands_count) {
    Ids matched_command_ids = {0};
    size_t command_len = strlen(command);
    char command_lower[1024] = {0};

    //
    size_t max = sizeof(command_lower) - 1;
    size_t copy_len = command_len > max ? max : command_len;
    for (size_t i=0;i<copy_len;++i) command_lower[i]=tolower((unsigned char)command[i]);
    command_lower[copy_len]=0;
    if (command_len > max) command_len = copy_len; // or treat as truncated
    //

    for (int i = 0; i < commands_count; ++i) {
        const char *c = commands[i];
        size_t c_len = strlen(c);
        if (str_starts_with(c, command_lower)) {
            if ((command_len > c_len && command_lower[c_len] == ' ')
              || command_len <= c_len) {
                darr_append(matched_command_ids, i);
            }
        }
    }

    return matched_command_ids;
}


// Timer and Alarm
void update_timer(Timer *t, float dt) {
		t->time += dt;
}

bool on_alarm(Alarm *a, float dt) {
		update_timer(&a->timer, dt);

		if (a->timer.time >= a->alarm_time) {
            a->timer.time = 0;
            if (a->once) {
                if (!a->done) {
                    a->done = true;
                    return true;
                }
            } else {
                return true;
            }
		}
		return false;
}

#endif // ENGINE_IMPLEMENTATION
