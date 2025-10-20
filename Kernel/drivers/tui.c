#include "../../Lib/include/tui.h"
#include "../../Lib/include/screen.h"

// Direct VGA memory access
static unsigned char* vga = (unsigned char*)VGA_MEMORY;

// ============= UTILITY FUNCTIONS =============

void tui_init() {
    screen_clear();
}

void tui_draw_char_at(int x, int y, char c, unsigned char color) {
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) return;
    int offset = (y * VGA_WIDTH + x) * 2;
    vga[offset] = c;
    vga[offset + 1] = color;
}

void tui_fill_rect(int x, int y, int width, int height, char c, unsigned char color) {
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            tui_draw_char_at(x + col, y + row, c, color);
        }
    }
}

// ============= DRAWING PRIMITIVES =============

void tui_draw_box(int x, int y, int width, int height, unsigned char color) {
    // Corners
    tui_draw_char_at(x, y, TUI_TL_CORNER, color);
    tui_draw_char_at(x + width - 1, y, TUI_TR_CORNER, color);
    tui_draw_char_at(x, y + height - 1, TUI_BL_CORNER, color);
    tui_draw_char_at(x + width - 1, y + height - 1, TUI_BR_CORNER, color);
    
    // Horizontal lines
    for (int i = 1; i < width - 1; i++) {
        tui_draw_char_at(x + i, y, TUI_H_LINE, color);
        tui_draw_char_at(x + i, y + height - 1, TUI_H_LINE, color);
    }
    
    // Vertical lines
    for (int i = 1; i < height - 1; i++) {
        tui_draw_char_at(x, y + i, TUI_V_LINE, color);
        tui_draw_char_at(x + width - 1, y + i, TUI_V_LINE, color);
    }
}

void tui_draw_line_h(int x, int y, int length, unsigned char ch, unsigned char color) {
    for (int i = 0; i < length; i++) {
        tui_draw_char_at(x + i, y, ch, color);
    }
}

void tui_draw_line_v(int x, int y, int length, unsigned char ch, unsigned char color) {
    for (int i = 0; i < length; i++) {
        tui_draw_char_at(x, y + i, ch, color);
    }
}

// ============= WINDOW MANAGEMENT =============

void tui_draw_window(tui_window_t* win) {
    unsigned char border_attr = (win->bg_color << 4) | win->border_color;
    
    // Draw border
    tui_draw_box(win->x, win->y, win->width, win->height, border_attr);
    
    // Draw title if exists
    if (win->title[0] != '\0') {
        int title_len = 0;
        while (win->title[title_len] && title_len < 60) title_len++;
        
        int title_start = win->x + (win->width - title_len - 2) / 2;
        
        unsigned char title_attr = (win->bg_color << 4) | COLOR_YELLOW;
        tui_draw_char_at(title_start, win->y, ' ', title_attr);
        
        for (int i = 0; i < title_len; i++) {
            tui_draw_char_at(title_start + 1 + i, win->y, win->title[i], title_attr);
        }
        
        tui_draw_char_at(title_start + title_len + 1, win->y, ' ', title_attr);
    }
    
    // Clear content area
    unsigned char content_attr = (win->bg_color << 4) | win->text_color;
    for (int row = 1; row < win->height - 1; row++) {
        for (int col = 1; col < win->width - 1; col++) {
            tui_draw_char_at(win->x + col, win->y + row, ' ', content_attr);
        }
    }
}

void tui_clear_window_content(tui_window_t* win) {
    unsigned char content_attr = (win->bg_color << 4) | win->text_color;
    for (int row = 1; row < win->height - 1; row++) {
        for (int col = 1; col < win->width - 1; col++) {
            tui_draw_char_at(win->x + col, win->y + row, ' ', content_attr);
        }
    }
}

void tui_write_in_window(tui_window_t* win, int x, int y, const char* text, unsigned char color) {
    int write_x = win->x + 1 + x;
    int write_y = win->y + 1 + y;
    
    int i = 0;
    while (text[i] && write_x < win->x + win->width - 1) {
        tui_draw_char_at(write_x, write_y, text[i], color);
        write_x++;
        i++;
    }
}

// ============= STATUS BAR =============

void tui_draw_statusbar(tui_statusbar_t* bar) {
    unsigned char attr = (bar->bg_color << 4) | bar->fg_color;
    
    // Fill entire top line with background
    for (int i = 0; i < VGA_WIDTH; i++) {
        tui_draw_char_at(i, 0, ' ', attr);
    }
    
    // Left text
    int pos = 1;
    for (int i = 0; bar->left_text[i] && pos < 28; i++) {
        tui_draw_char_at(pos++, 0, bar->left_text[i], attr);
    }
    
    // Center text
    int center_len = 0;
    while (bar->center_text[center_len]) center_len++;
    pos = (VGA_WIDTH - center_len) / 2;
    for (int i = 0; bar->center_text[i] && pos < VGA_WIDTH - 1; i++) {
        tui_draw_char_at(pos++, 0, bar->center_text[i], attr);
    }
    
    // Right text
    int right_len = 0;
    while (bar->right_text[right_len]) right_len++;
    pos = VGA_WIDTH - right_len - 1;
    for (int i = 0; bar->right_text[i] && pos < VGA_WIDTH - 1; i++) {
        tui_draw_char_at(pos++, 0, bar->right_text[i], attr);
    }
}

void tui_update_statusbar(tui_statusbar_t* bar, const char* left, const char* center, const char* right) {
    // Copy strings
    int i = 0;
    while (left && left[i] && i < 29) {
        bar->left_text[i] = left[i];
        i++;
    }
    bar->left_text[i] = '\0';
    
    i = 0;
    while (center && center[i] && i < 29) {
        bar->center_text[i] = center[i];
        i++;
    }
    bar->center_text[i] = '\0';
    
    i = 0;
    while (right && right[i] && i < 29) {
        bar->right_text[i] = right[i];
        i++;
    }
    bar->right_text[i] = '\0';
    
    tui_draw_statusbar(bar);
}

// ============= PROGRESS BAR =============

void tui_draw_progress_bar(int x, int y, int width, int percent, unsigned char color) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    
    int filled = (width * percent) / 100;
    
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            tui_draw_char_at(x + i, y, TUI_BLOCK_FULL, color);
        } else {
            tui_draw_char_at(x + i, y, TUI_BLOCK_LIGHT, (color & 0xF0) | COLOR_DARK_GREY);
        }
    }
}

// ============= LIST RENDERING =============

void tui_draw_list(tui_window_t* win, tui_list_item_t* items, int item_count, int selected_idx) {
    int content_height = win->height - 2;
    int start_idx = win->scroll_offset;
    int end_idx = start_idx + content_height;
    
    if (end_idx > item_count) end_idx = item_count;
    
    for (int i = start_idx; i < end_idx; i++) {
        int row = i - start_idx;
        unsigned char attr;
        
        if (i == selected_idx) {
            // Highlighted selection
            attr = (COLOR_BLUE << 4) | COLOR_WHITE;
        } else {
            attr = (win->bg_color << 4) | items[i].color;
        }
        
        // Draw selection bar
        for (int col = 1; col < win->width - 1; col++) {
            tui_draw_char_at(win->x + col, win->y + 1 + row, ' ', attr);
        }
        
        // Draw item icon
        char icon = (items[i].type == 'd') ? TUI_ARROW_RIGHT : ' ';
        tui_draw_char_at(win->x + 2, win->y + 1 + row, icon, attr);
        
        // Draw item name
        int name_x = win->x + 4;
        for (int j = 0; items[i].name[j] && j < win->width - 6; j++) {
            tui_draw_char_at(name_x + j, win->y + 1 + row, items[i].name[j], attr);
        }
    }
}

// ============= MESSAGE BOX =============

void tui_show_message_box(const char* title, const char* message) {
    int box_width = 50;
    int box_height = 8;
    int box_x = (VGA_WIDTH - box_width) / 2;
    int box_y = (VGA_HEIGHT - box_height) / 2;
    
    tui_window_t msg_win = {
        .x = box_x,
        .y = box_y,
        .width = box_width,
        .height = box_height,
        .border_color = COLOR_YELLOW,
        .bg_color = COLOR_BLUE,
        .text_color = COLOR_WHITE,
        .active = 1
    };
    
    // Copy title
    int i = 0;
    while (title[i] && i < 63) {
        msg_win.title[i] = title[i];
        i++;
    }
    msg_win.title[i] = '\0';
    
    tui_draw_window(&msg_win);
    
    // Draw message centered
    int msg_len = 0;
    while (message[msg_len]) msg_len++;
    int msg_x = (box_width - 2 - msg_len) / 2;
    int msg_y = box_height / 2 - 1;
    
    tui_write_in_window(&msg_win, msg_x, msg_y, message, COLOR_WHITE);
    
    // Draw "Press any key" at bottom
    const char* prompt = "Press any key...";
    int prompt_len = 16;
    int prompt_x = (box_width - 2 - prompt_len) / 2;
    tui_write_in_window(&msg_win, prompt_x, box_height - 3, prompt, COLOR_LIGHT_GREY);
}
