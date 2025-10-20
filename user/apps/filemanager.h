#ifndef FILEMANAGER_H
#define FILEMANAGER_H

// File manager initialization and main loop
void filemanager_init();
void filemanager_run();
void filemanager_exit();

// Navigation commands
void filemanager_move_up();
void filemanager_move_down();
void filemanager_page_up();
void filemanager_page_down();
void filemanager_select_item();

// File operations (placeholders for now since we don't have filesystem)
void filemanager_open_file();
void filemanager_delete_file();
void filemanager_rename_file();
void filemanager_create_directory();

// Display refresh
void filemanager_redraw();

#endif
