#include "../../Lib/include/screen.h"
#include "../../Lib/include/keyboard.h"
#include "../apps/filemanager.h"

#define MAX_COMMAND_LENGTH 512
#define MAX_HISTORY 100
#define MAX_ALIASES 50
#define MAX_VARIABLES 50

static char command_buffer[MAX_COMMAND_LENGTH];
static int cmd_index = 0;

static char history[MAX_HISTORY][MAX_COMMAND_LENGTH];
static int history_count = 0;

typedef struct {
    char name[32];
    char value[256];
    int active;
} alias_t;
static alias_t aliases[MAX_ALIASES];
static int alias_count = 0;

typedef struct {
    char name[32];
    char value[256];
} var_t;
static var_t env_vars[MAX_VARIABLES];
static int var_count = 0;

static struct {
    unsigned int command_count;
} shell_state = {0};

int detect_cpu_safe(char* vendor) {
    unsigned int eax, ebx, ecx, edx;
    
    int cpuid_works = 0;
    __asm__ __volatile__(
        "pushfl\n\t"
        "pushfl\n\t"
        "xorl (%%esp), %%eax\n\t"
        "popfl\n\t"
        "pushfl\n\t"
        "popl %%eax\n\t"
        "xorl (%%esp), %%eax\n\t"
        "popfl\n\t"
        : "=a"(cpuid_works)
    );
    
    if (!(cpuid_works & 0x00200000)) {
        return 0;
    }
    
    __asm__ __volatile__(
        "cpuid"
        : "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );
    
    for (int i = 0; i < 4; i++) {
        vendor[i] = (ebx >> (i * 8)) & 0xFF;
        vendor[i + 4] = (edx >> (i * 8)) & 0xFF;
        vendor[i + 8] = (ecx >> (i * 8)) & 0xFF;
    }
    vendor[12] = '\0';
    return 1;
}

int str_len(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

int str_cmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void str_cpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

int starts_with(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) return 0;
    }
    return 1;
}

char* str_trim(char* str) {
    while (*str == ' ' || *str == '\t') str++;
    if (*str == 0) return str;
    char* end = str + str_len(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t')) end--;
    *(end + 1) = '\0';
    return str;
}

void print_int(int num) {
    if (num == 0) { screen_putchar('0'); return; }
    if (num < 0) { screen_putchar('-'); num = -num; }
    char buf[12];
    int i = 0;
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i > 0) screen_putchar(buf[--i]);
}

void env_set(const char* name, const char* value) {
    for (int i = 0; i < var_count; i++) {
        if (str_cmp(env_vars[i].name, name) == 0) {
            str_cpy(env_vars[i].value, value);
            return;
        }
    }
    if (var_count < MAX_VARIABLES) {
        str_cpy(env_vars[var_count].name, name);
        str_cpy(env_vars[var_count].value, value);
        var_count++;
    }
}

const char* env_get(const char* name) {
    for (int i = 0; i < var_count; i++) {
        if (str_cmp(env_vars[i].name, name) == 0) {
            return env_vars[i].value;
        }
    }
    return 0;
}

void alias_add(const char* name, const char* value) {
    for (int i = 0; i < alias_count; i++) {
        if (str_cmp(aliases[i].name, name) == 0) {
            str_cpy(aliases[i].value, value);
            aliases[i].active = 1;
            return;
        }
    }
    if (alias_count < MAX_ALIASES) {
        str_cpy(aliases[alias_count].name, name);
        str_cpy(aliases[alias_count].value, value);
        aliases[alias_count].active = 1;
        alias_count++;
    }
}

const char* alias_get(const char* name) {
    for (int i = 0; i < alias_count; i++) {
        if (aliases[i].active && str_cmp(aliases[i].name, name) == 0) {
            return aliases[i].value;
        }
    }
    return 0;
}

void history_add(const char* cmd) {
    if (history_count >= MAX_HISTORY) {
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            str_cpy(history[i], history[i + 1]);
        }
        history_count--;
    }
    str_cpy(history[history_count], cmd);
    history_count++;
}

void cmd_help() {
    screen_set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_println("╔═══════════════════════════════════════════════╗");
    screen_println("║    SEPPUKU OS Professional Shell v2.0         ║");
    screen_println("╚═══════════════════════════════════════════════╝");
    
    screen_set_color(COLOR_YELLOW, COLOR_BLACK);
    screen_println("\nBASIC:");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    screen_println("  help           - Show this help");
    screen_println("  clear, cls     - Clear screen");
    screen_println("  echo <text>    - Display text");
    screen_println("  history        - Show command history");
    
    screen_set_color(COLOR_YELLOW, COLOR_BLACK);
    screen_println("\nSYSTEM:");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    screen_println("  sysinfo        - System information");
    screen_println("  cpuinfo        - CPU details");
    screen_println("  banner <txt>   - Display banner");
    screen_println("  calc <expr>    - Calculator");
    
    screen_set_color(COLOR_YELLOW, COLOR_BLACK);
    screen_println("\nAPPLICATIONS:");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    screen_println("  fm, files      - File Manager (TUI)");
    
    screen_set_color(COLOR_YELLOW, COLOR_BLACK);
    screen_println("\nENVIRONMENT:");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    screen_println("  set var=val    - Set variable");
    screen_println("  env            - List variables");
    screen_println("  alias n=cmd    - Create alias");
    
    screen_set_color(COLOR_YELLOW, COLOR_BLACK);
    screen_println("\nPOWER:");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    screen_println("  reboot         - Reboot system");
    screen_println("  halt           - Halt system");
}

void cmd_sysinfo() {
    screen_set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_println("═══════════════════════════════════════════");
    screen_println("      SYSTEM INFORMATION");
    screen_println("═══════════════════════════════════════════");
    
    screen_set_color(COLOR_YELLOW, COLOR_BLACK);
    screen_println("\n[CPU]");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    
    char cpu_vendor[13] = {0};
    if (detect_cpu_safe(cpu_vendor)) {
        screen_print("  Vendor: ");
        screen_println(cpu_vendor);
    } else {
        screen_println("  Vendor: x86 Compatible");
    }
    screen_println("  Architecture: i386 (32-bit)");
    
    screen_set_color(COLOR_YELLOW, COLOR_BLACK);
    screen_println("\n[MEMORY]");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    screen_println("  Available: Check with 'meminfo'");
    
    screen_set_color(COLOR_YELLOW, COLOR_BLACK);
    screen_println("\n[HARDWARE]");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    screen_println("  Display: VGA Text Mode 80x25");
    screen_println("  Keyboard: PS/2");
    screen_println("  Boot: Legacy BIOS");
    
    screen_set_color(COLOR_YELLOW, COLOR_BLACK);
    screen_println("\n[KERNEL]");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    screen_println("  Name: SeppukuKernel");
    screen_println("  Version: 1.2");
    screen_println("  Type: Monolithic");
    
    screen_set_color(COLOR_YELLOW, COLOR_BLACK);
    screen_println("\n[SHELL STATS]");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    screen_print("  Commands: ");
    print_int(shell_state.command_count);
    screen_putchar('\n');
    screen_print("  History: ");
    print_int(history_count);
    screen_putchar('\n');
    screen_print("  Aliases: ");
    print_int(alias_count);
    screen_putchar('\n');
}

void cmd_cpuinfo() {
    screen_set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_println("CPU Information:");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    
    char cpu_vendor[13] = {0};
    if (detect_cpu_safe(cpu_vendor)) {
        screen_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        screen_print("  Vendor: ");
        screen_set_color(COLOR_YELLOW, COLOR_BLACK);
        screen_println(cpu_vendor);
    } else {
        screen_set_color(COLOR_LIGHT_GREY, COLOR_BLACK);
        screen_println("  CPUID not supported on this CPU");
    }
    
    screen_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_println("\n  Architecture:");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    screen_println("    - i386 (32-bit)");
    screen_println("    - Protected Mode");
    screen_println("    - Real Address Mode capable");
}

void cmd_echo(const char* text) {
    if (text[0] == ' && text[1] != '\0') {
        const char* val = env_get(text + 1);
        if (val) {
            screen_set_color(COLOR_YELLOW, COLOR_BLACK);
            screen_println(val);
        } else {
            screen_set_color(COLOR_RED, COLOR_BLACK);
            screen_println("(undefined)");
        }
    } else {
        screen_set_color(COLOR_YELLOW, COLOR_BLACK);
        screen_println(text);
    }
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
}

void cmd_history_show() {
    screen_set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_println("Command History:");
    screen_set_color(COLOR_LIGHT_GREY, COLOR_BLACK);
    for (int i = 0; i < history_count; i++) {
        screen_print("  ");
        print_int(i + 1);
        screen_print("  ");
        screen_println(history[i]);
    }
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
}

void cmd_env() {
    screen_set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_println("Environment Variables:");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    for (int i = 0; i < var_count; i++) {
        screen_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        screen_print("  ");
        screen_print(env_vars[i].name);
        screen_set_color(COLOR_WHITE, COLOR_BLACK);
        screen_print("=");
        screen_set_color(COLOR_YELLOW, COLOR_BLACK);
        screen_println(env_vars[i].value);
    }
    if (var_count == 0) {
        screen_set_color(COLOR_LIGHT_GREY, COLOR_BLACK);
        screen_println("  (no variables set)");
    }
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
}

void cmd_alias_list() {
    screen_set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_println("Active Aliases:");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    int count = 0;
    for (int i = 0; i < alias_count; i++) {
        if (aliases[i].active) {
            screen_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
            screen_print("  ");
            screen_print(aliases[i].name);
            screen_set_color(COLOR_WHITE, COLOR_BLACK);
            screen_print(" = ");
            screen_set_color(COLOR_YELLOW, COLOR_BLACK);
            screen_println(aliases[i].value);
            count++;
        }
    }
    if (count == 0) {
        screen_set_color(COLOR_LIGHT_GREY, COLOR_BLACK);
        screen_println("  (no aliases)");
    }
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
}

void cmd_calc(const char* expr) {
    char op = 0;
    int num1 = 0, num2 = 0;
    int i = 0;
    
    while (expr[i] >= '0' && expr[i] <= '9') {
        num1 = num1 * 10 + (expr[i] - '0');
        i++;
    }
    
    while (expr[i] == ' ') i++;
    if (expr[i] == '+' || expr[i] == '-' || expr[i] == '*' || expr[i] == '/') {
        op = expr[i++];
    }
    
    while (expr[i] == ' ') i++;
    while (expr[i] >= '0' && expr[i] <= '9') {
        num2 = num2 * 10 + (expr[i] - '0');
        i++;
    }
    
    if (op) {
        int result = 0;
        switch (op) {
            case '+': result = num1 + num2; break;
            case '-': result = num1 - num2; break;
            case '*': result = num1 * num2; break;
            case '/': 
                if (num2 != 0) result = num1 / num2;
                else {
                    screen_set_color(COLOR_RED, COLOR_BLACK);
                    screen_println("Error: Division by zero");
                    screen_set_color(COLOR_WHITE, COLOR_BLACK);
                    return;
                }
                break;
        }
        screen_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        print_int(result);
        screen_putchar('\n');
        screen_set_color(COLOR_WHITE, COLOR_BLACK);
    } else {
        screen_set_color(COLOR_RED, COLOR_BLACK);
        screen_println("Usage: calc <num> <op> <num>");
        screen_set_color(COLOR_LIGHT_GREY, COLOR_BLACK);
        screen_println("Example: calc 10 + 5");
        screen_set_color(COLOR_WHITE, COLOR_BLACK);
    }
}

void cmd_banner(const char* text) {
    screen_set_color(COLOR_LIGHT_MAGENTA, COLOR_BLACK);
    screen_println("╔══════════════════════════════════╗");
    screen_print("║  ");
    screen_print(text);
    int padding = 32 - str_len(text);
    for (int i = 0; i < padding; i++) screen_putchar(' ');
    screen_println("║");
    screen_println("╚══════════════════════════════════╝");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
}

void cmd_reboot() {
    screen_set_color(COLOR_YELLOW, COLOR_BLACK);
    screen_println("Rebooting...");
    unsigned char temp;
    __asm__ __volatile__("inb %1, %0" : "=a"(temp) : "Nd"((unsigned short)0x64));
    while (temp & 0x02) {
        __asm__ __volatile__("inb %1, %0" : "=a"(temp) : "Nd"((unsigned short)0x64));
    }
    __asm__ __volatile__("outb %0, %1" : : "a"((unsigned char)0xFE), "Nd"((unsigned short)0x64));
    __asm__ __volatile__("hlt");
}

void shell_execute(const char* cmd) {
    if (cmd[0] == '\0') return;
    
    char* trimmed = str_trim((char*)cmd);
    if (trimmed[0] == '\0') return;
    
    history_add(trimmed);
    shell_state.command_count++;
    
    const char* alias_val = alias_get(trimmed);
    if (alias_val) {
        shell_execute(alias_val);
        return;
    }
    
    char cmd_name[64] = {0};
    const char* args = trimmed;
    int i = 0;
    while (*args && *args != ' ' && i < 63) {
        cmd_name[i++] = *args++;
    }
    cmd_name[i] = '\0';
    while (*args == ' ') args++;
    
    if (str_cmp(cmd_name, "help") == 0) cmd_help();
    else if (str_cmp(cmd_name, "clear") == 0 || str_cmp(cmd_name, "cls") == 0) screen_clear();
    else if (str_cmp(cmd_name, "echo") == 0) cmd_echo(args);
    else if (str_cmp(cmd_name, "history") == 0) cmd_history_show();
    else if (str_cmp(cmd_name, "sysinfo") == 0) cmd_sysinfo();
    else if (str_cmp(cmd_name, "cpuinfo") == 0) cmd_cpuinfo();
    else if (str_cmp(cmd_name, "env") == 0) cmd_env();
    else if (str_cmp(cmd_name, "calc") == 0) cmd_calc(args);
    else if (str_cmp(cmd_name, "banner") == 0) cmd_banner(args);
    else if (str_cmp(cmd_name, "reboot") == 0) cmd_reboot();
    else if (str_cmp(cmd_name, "fm") == 0 || str_cmp(cmd_name, "files") == 0) {
        screen_set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
        screen_println("\nLaunching File Manager...");
        screen_set_color(COLOR_WHITE, COLOR_BLACK);
        
        // Small delay for visual effect
        for (volatile int j = 0; j < 10000000; j++);
        
        // Launch file manager
        filemanager_run();
        
        // When returning from file manager, show prompt again
        screen_clear();
        shell_init();
        return;
    }
    else if (str_cmp(cmd_name, "halt") == 0) {
        screen_set_color(COLOR_YELLOW, COLOR_BLACK);
        screen_println("System halted.");
        while(1) __asm__ __volatile__("hlt");
    }
    else if (str_cmp(cmd_name, "alias") == 0) {
        if (*args) {
            char name[32], value[256];
            const char* eq = args;
            while (*eq && *eq != '=') eq++;
            if (*eq == '=') {
                int j = 0;
                while (args < eq && j < 31) name[j++] = *args++;
                name[j] = '\0';
                args = eq + 1;
                str_cpy(value, args);
                alias_add(name, value);
                screen_set_color(COLOR_GREEN, COLOR_BLACK);
                screen_println("✓ Alias created");
                screen_set_color(COLOR_WHITE, COLOR_BLACK);
            }
        } else cmd_alias_list();
    }
    else if (str_cmp(cmd_name, "set") == 0) {
        if (*args) {
            char name[32], value[256];
            const char* eq = args;
            while (*eq && *eq != '=') eq++;
            if (*eq == '=') {
                int j = 0;
                while (args < eq && j < 31) name[j++] = *args++;
                name[j] = '\0';
                args = eq + 1;
                str_cpy(value, args);
                env_set(name, value);
                screen_set_color(COLOR_GREEN, COLOR_BLACK);
                screen_println("✓ Variable set");
                screen_set_color(COLOR_WHITE, COLOR_BLACK);
            }
        }
    }
    else {
        screen_set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        screen_print("Unknown: ");
        screen_println(cmd_name);
        screen_set_color(COLOR_LIGHT_GREY, COLOR_BLACK);
        screen_println("Type 'help' for commands");
        screen_set_color(COLOR_WHITE, COLOR_BLACK);
    }
}

void shell_prompt() {
    screen_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_print("root");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    screen_print("@");
    screen_set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_print("seppuku");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    screen_print(":");
    screen_set_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
    screen_print("~");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    screen_print("$ ");
}

void shell_handle_key(char c) {
    if (c == '\n') {
        screen_putchar('\n');
        command_buffer[cmd_index] = '\0';
        shell_execute(command_buffer);
        cmd_index = 0;
        shell_prompt();
    } else if (c == '\b') {
        if (cmd_index > 0) {
            cmd_index--;
            screen_putchar('\b');
        }
    } else if (cmd_index < MAX_COMMAND_LENGTH - 1) {
        command_buffer[cmd_index++] = c;
        screen_putchar(c);
    }
}

void shell_init() {
    env_set("OS", "SEPPUKU");
    env_set("VER", "1.2");
    alias_add("cls", "clear");
    
    screen_set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_println("\n╔════════════════════════════════════════╗");
    screen_set_color(COLOR_LIGHT_RED, COLOR_BLACK);
    screen_println("║   SEPPUKU OS Shell v2.0                ║");
    screen_set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_println("╚════════════════════════════════════════╝");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    
    char cpu_vendor[13] = {0};
    if (detect_cpu_safe(cpu_vendor)) {
        screen_set_color(COLOR_LIGHT_GREY, COLOR_BLACK);
        screen_print("\nCPU: ");
        screen_println(cpu_vendor);
    }
    
    screen_set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_println("\nType 'help' for commands.\n");
    screen_set_color(COLOR_WHITE, COLOR_BLACK);
    
    shell_prompt();
}

void shell_run() {
    shell_init();
    
    while (1) {
        if (keyboard_available()) {
            char c = keyboard_getchar();
            shell_handle_key(c);
        }
        __asm__ __volatile__("hlt");
    }
}
