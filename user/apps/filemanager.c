#include "../../Lib/include/tui.h"
#include "../../Lib/include/keyboard.h"
#include "../../Lib/include/screen.h"
#include "filemanager.h"

// File manager state
static int fm_running = 0;
static int fm_selected_index = 0;
static int fm_item_count = 0;

// Windows
static tui_window_t fm_main_window;
static tui_statusbar_t fm_statusbar;

// Mock filesystem (since we don't have real FS yet)
#define MAX_ITEMS 100
static tui_list_item_t fm_items[MAX_ITEMS];

// Helper: string copy
static void fm_strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

// Helper: string length
static int fm_strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

// Helper: integer to string
static void fm_itoa(int num, char* str) {
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    int i = 0;
    int is_negative = 0;
    
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }
    
    while (num > 0) {
        str[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    if (is_negative) str[i++] = '-';
    str[i] = '\0';
    
    // Reverse
    for (int j = 0; j < i / 2; j++) {
        char temp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = temp;
    }
}

// Initialize mock file system
static void fm_init_mock_fs() {
    fm_item_count = 0;
    
    // Add some mock directories
    fm_strcpy(fm_items[fm_item_count].name, ".. (Parent)");
    fm_items[fm_item_count].type = 'd';
    fm_items[fm_item_count].size = 0;
    fm_items[fm_item_count].color = COLOR_LIGHT_CYAN;
    fm_item_count++;
    
    fm_strcpy(fm_items[fm_item_count].name, "Documents");
    fm_items[fm_item_count].type = 'd';
    fm_items[fm_item_count].size = 0;
    fm_items[fm_item_count].color = COLOR_LIGHT_BLUE;
    fm_item_count++;
    
    fm_strcpy(fm_items[fm_item_count].name, "Programs");
    fm_items[fm_item_count].type = 'd';
    fm_items[fm_item_count].size = 0;
    fm_items[fm_item_count].color = COLOR_LIGHT_BLUE;
    fm_item_count++;
    
    fm_strcpy(fm_items[fm_item_count].name, "System");
    fm_items[fm_item_count].type = 'd';
    fm_items[fm_item_count].size = 0;
    fm_items[fm_item_count].color = COLOR_LIGHT_BLUE;
    fm_item_count++;
    
    // Add some mock files
    fm_strcpy(fm_items[fm_item_count].name, "kernel.bin");
    fm_items[fm_item_count].type = 'f';
    fm_items[fm_item_count].size = 4096;
    fm_items[fm_item_count].color = COLOR_WHITE;
    fm_item_count++;
    
    fm_strcpy(fm_items[fm_item_count].name, "boot.bin");
    fm_items[fm_item_count].type = 'f';
    fm_items[fm_item_count].size = 512;
    fm_items[fm_item_count].color = COLOR_WHITE;
    fm_item_count++;
    
    fm_strcpy(fm_items[fm_item_count].name, "README.txt");
    fm_items[fm_item_count].type = 'f';
    fm_items[fm_item_count].size = 1024;
    fm_items[fm_item_count].color = COLOR_LIGHT_GREY;
    fm_item_count++;
    
    fm_strcpy(fm_items[fm_item_count].name, "config.sys");
    fm_items[fm_item_count].type = 'f';
    fm_items[fm_item_count].size = 256;
    fm_items[fm_item_count].color = COLOR_YELLOW;
    fm_item_count++;
    
    fm_strcpy(fm_items[fm_item_count].name, "seppuku.log");
    fm_items[fm_item_count].type = 'f';
    fm_items[fm_item_count].size = 2048;
    fm_items[fm_item_count].color = COLOR_LIGHT_GREY;
    fm_item_count++;
}

// Initialize file manager
void filemanager_init() {
    tui_init();
    
    // Setup main window
    fm_main_window.x = 2;
    fm_main_window.y = 2;
    fm_main_window.width = 76;
    fm_main_window.height = 20;
    fm_strcpy(fm_main_window.title, "SEPPUKU File Manager");
    fm_main_window.border_color = COLOR_LIGHT_CYAN;
    fm_main_window.bg_color = COLOR_BLACK;
    fm_main_window.text_color = COLOR_WHITE;
    fm_main_window.active = 1;
    fm_main_window.scroll_offset = 0;
    
    // Setup status bar
    fm_statusbar.bg_color = COLOR_BLUE;
    fm_statusbar.fg_color = COLOR_WHITE;
    fm_strcpy(fm_statusbar.left_text, " SEPPUKU OS v1.2");
    fm_strcpy(fm_statusbar.center_text, "File Manager");
    fm_strcpy(fm_statusbar.right_text, "F1=Help Q=Quit ");
    
    // Initialize mock filesystem
    fm_init_mock_fs();
    fm_selected_index = 0;
    fm_running = 1;
    
    // Initial draw
    filemanager_redraw();
}

// Redraw the file manager interface
void filemanager_redraw() {
    // Draw status bar
    tui_draw_statusbar(&fm_statusbar);
    
    // Draw main window
    tui_draw_window(&fm_main_window);
    
    // Draw file list
    tui_draw_list(&fm_main_window, fm_items, fm_item_count, fm_selected_index);
    
    // Draw help bar at bottom
    tui_window_t help_bar;
    help_bar.x = 2;
    help_bar.y = 22;
    help_bar.width = 76;
    help_bar.height = 3;
    help_bar.title[0] = '\0';
    help_bar.border_color = COLOR_YELLOW;
    help_bar.bg_color = COLOR_BLACK;
    help_bar.text_color = COLOR_WHITE;
    help_bar.active = 0;
    help_bar.scroll_offset = 0;
    
    tui_draw_window(&help_bar);
    tui_write_in_window(&help_bar, 1, 0, "↑↓=Navigate  ENTER=Open  DEL=Delete  F2=Rename  Q=Quit", COLOR_YELLOW);
    
    // Update status bar with current selection info
    if (fm_selected_index >= 0 && fm_selected_index < fm_item_count) {
        char status[30];
        char size_str[12];
        
        // Build status message
        int idx = 0;
        status[idx++] = ' ';
        status[idx++] = 'I';
        status[idx++] = 't';
        status[idx++] = 'e';
        status[idx++] = 'm';
        status[idx++] = ' ';
        
        // Add item number
        char num_str[12];
        fm_itoa(fm_selected_index + 1, num_str);
        for (int i = 0; num_str[i]; i++) status[idx++] = num_str[i];
        status[idx++] = '/';
        fm_itoa(fm_item_count, num_str);
        for (int i = 0; num_str[i]; i++) status[idx++] = num_str[i];
        status[idx++] = ' ';
        status[idx] = '\0';
        
        tui_update_statusbar(&fm_statusbar, fm_statusbar.left_text, status, fm_statusbar.right_text);
    }
}

// Navigation
void filemanager_move_up() {
    if (fm_selected_index > 0) {
        fm_selected_index--;
        
        // Adjust scroll if needed
        if (fm_selected_index < fm_main_window.scroll_offset) {
            fm_main_window.scroll_offset = fm_selected_index;
        }
        
        filemanager_redraw();
    }
}

void filemanager_move_down() {
    if (fm_selected_index < fm_item_count - 1) {
        fm_selected_index++;
        
        // Adjust scroll if needed
        int visible_items = fm_main_window.height - 2;
        if (fm_selected_index >= fm_main_window.scroll_offset + visible_items) {
            fm_main_window.scroll_offset = fm_selected_index - visible
