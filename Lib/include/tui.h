#ifndef TUI_H
#define TUI_H

#include "screen.h"

// Box drawing characters (CP437 / Extended ASCII)
#define TUI_TL_CORNER  218  // ┌
#define TUI_TR_CORNER  191  // ┐
#define TUI_BL_CORNER  192  // └
#define TUI_BR_CORNER  217  // ┘
#define TUI_H_LINE     196  // ─
#define TUI_V_LINE     179  // │
#define TUI_T_DOWN     194  // ┬
#define TUI_T_UP       193  // ┴
#define TUI_T_RIGHT    195  // ├
#define TUI_T_LEFT     180  // ┤
#define TUI_CROSS      197  // ┼

// Block characters for progress bars, etc.
#define TUI_BLOCK_FULL  219  // █
#define TUI_BLOCK_DARK  178  // ▒
#define TUI_BLOCK_MED   177  // ▓
#define TUI_BLOCK_LIGHT 176  // ░

// Arrow characters
#define TUI_ARROW_UP    24   // ↑
#define TUI_ARROW_DOWN  25   // ↓
#define TUI_ARROW_RIGHT 26   // →
#define TUI_ARROW_LEFT  27   // ←

// TUI Window structure
typedef struct {
    int x, y;
    int width, height;
    char title[64];
    unsigned char border_color;
    unsigned char bg_color;
    unsigned char text_color;
    int active;
    int scroll_offset;
} tui_window_t;

// Status bar structure
typedef struct {
    char left_text[30];
    char center_text[30];
    char right_text[30];
    unsigned char bg_color;
    unsigned char fg_color;
} tui_statusbar_t;

// Menu item structure
typedef struct {
    char text[40];
    int selected;
    void (*action)(void);
} tui_menu_item_t;

// List item structure (for file manager)
typedef struct {
    char name[60];
    char type;  // 'f' = file, 'd' = directory
    int size;
    unsigned char color;
} tui_list_item_t;

// ============= FUNCTION PROTOTYPES =============

// Window management
void tui_init();
void tui_draw_window(tui_window_t* win);
void tui_clear_window_content(tui_window_t* win);
void tui_write_in_window(tui_window_t* win, int x, int y, const char* text, unsigned char color);

// Status bar
void tui_draw_statusbar(tui_statusbar_t* bar);
void tui_update_statusbar(tui_statusbar_t* bar, const char* left, const char* center, const char* right);

// Drawing primitives
void tui_draw_box(int x, int y, int width, int height, unsigned char color);
void tui_draw_line_h(int x, int y, int length, unsigned char ch, unsigned char color);
void tui_draw_line_v(int x, int y, int length, unsigned char ch, unsigned char color);
void tui_draw_progress_bar(int x, int y, int width, int percent, unsigned char color);

// List/Menu rendering
void tui_draw_list(tui_window_t* win, tui_list_item_t* items, int item_count, int selected_idx);
void tui_draw_menu(int x, int y, tui_menu_item_t* items, int item_count);

// Utility
void tui_draw_char_at(int x, int y, char c, unsigned char color);
void tui_fill_rect(int x, int y, int width, int height, char c, unsigned char color);

// Dialog boxes
void tui_show_message_box(const char* title, const char* message);
int tui_show_confirm_box(const char* title, const char* message);

#endif
