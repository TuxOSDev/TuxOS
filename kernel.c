/* kernel.c */

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef signed char int8_t;

#define VBE_DISPI_IOPORT_INDEX 0x01CE
#define VBE_DISPI_IOPORT_DATA  0x01CF
#define VBE_DISPI_INDEX_XRES        1
#define VBE_DISPI_INDEX_YRES        2
#define VBE_DISPI_INDEX_BPP          3
#define VBE_DISPI_INDEX_ENABLE      4
#define VBE_DISPI_DISABLED          0x00
#define VBE_DISPI_ENABLED           0x01
#define VBE_DISPI_LFB_ENABLED       0x40

static int screen_width  = 800;
static int screen_height = 600;
static int bytes_per_pixel = 4;

#define UI_TOP_OFFSET 30
#define MAX_ROWS 15
#define MAX_COLS 56

/* Colors */
#define COLOR_WHITE      0xFFFFFF
#define COLOR_BLACK      0x000000
#define COLOR_BLUE       0x1A1B26
#define COLOR_LIGHT_GRAY 0xD0D0D0
#define COLOR_DARK_GRAY  0x404040
#define COLOR_RED        0xFF0000
#define COLOR_TITLE_BLUE 0x34495E
#define COLOR_DESKTOP    0x3A74B8

#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_STATUS_PORT 0x64

void put_pixel(int x, int y, uint32_t color);
void draw_rect(int start_x, int start_y, int width, int height, uint32_t color);
void draw_text_absolute(int x_pos, int y_pos, const char* str, uint32_t fg, uint32_t bg);
void update_ui_chrome();
void handle_mouse_polling();
void shell_execute(const char* cmd, const char* args);
int strcmp(const char *a, const char *b);
int strlen(const char *s);
int strncmp(const char *a, const char *b, int n);
uint32_t parse_hex(const char* str);
void redraw_all();
void draw_windows();
void blit_bounding_box(int sx, int sy, int w, int h);
void blit_entire_screen();
void run_pong_game();

/* UI Window & State Tracker - Terminal */
static int window_open = 0;
static int window_x = 150;
static int window_y = 120;
static int window_width = 480;
static int window_height = 320;
static int menu_open = 0;

/* UI Window & State Tracker - About Dialog */
static int about_open = 0;
static int about_x = 260;
static int about_y = 200;
static int about_width = 280;
static int about_height = 140;

/* Drag Tracker Flags */
static int is_dragging = 0;
static int drag_offset_x = 0;
static int drag_offset_y = 0;

/* Persistent Terminal History Buffer */
static char terminal_history[MAX_ROWS][MAX_COLS + 1];
static int terminal_history_count = 0;

/* Terminal Shell Buffer Globals */
static char shell_input_buf[128];
static int shell_buf_pos = 0;
static int shell_shift_state = 0;

/* Game State Variable */
static int target_game_num = -1;

/* Mouse State variables */
static int mouse_x = 400;
static int mouse_y = 300;
static int old_mouse_x = 400;
static int old_mouse_y = 300;
static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];
static uint8_t prev_mouse_buttons = 0;

/* Buffer Pointers */
static uint32_t* framebuffer = 0;
static uint32_t* back_buffer = (uint32_t*)0x00400000;

static unsigned int boot_epoch = 125400;
static unsigned int random_seed = 42135;

/* FULL FONT BITMAP */
static const unsigned char font8x8[128][8] = {
    [' '] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['!'] = {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00},
    ['"'] = {0x36,0x36,0x36,0x00,0x00,0x00,0x00,0x00},
    ['#'] = {0x36,0x36,0x7f,0x36,0x7f,0x36,0x36,0x00},
    ['$'] = {0x12,0x3e,0x50,0x3c,0x12,0x7c,0x12,0x00},
    ['%'] = {0x63,0x66,0x30,0x18,0x0c,0x66,0x63,0x00},
    ['&'] = {0x38,0x6c,0x78,0x38,0x6c,0x6c,0x3a,0x00},
    ['\'']= {0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00},
    ['('] = {0x0c,0x18,0x30,0x30,0x30,0x18,0x0c,0x00},
    [')'] = {0x30,0x18,0x0c,0x0c,0x0c,0x18,0x30,0x00},
    ['*'] = {0x00,0x66,0x3c,0xff,0x3c,0x66,0x00,0x00},
    ['+'] = {0x00,0x18,0x18,0x7e,0x18,0x18,0x00,0x00},
    [','] = {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},
    ['-'] = {0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00},
    ['.'] = {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},
    ['/'] = {0x03,0x06,0x0c,0x18,0x30,0x60,0xc0,0x00},
    ['0'] = {0x3e,0x63,0x67,0x6f,0x7b,0x63,0x3e,0x00},
    ['1'] = {0x0c,0x1c,0x0c,0x0c,0x0c,0x0c,0x3e,0x00},
    ['2'] = {0x3e,0x63,0x06,0x1c,0x30,0x63,0x7f,0x00},
    ['3'] = {0x3e,0x63,0x06,0x1c,0x06,0x63,0x3e,0x00},
    ['4'] = {0x06,0x0e,0x1e,0x36,0x7f,0x06,0x06,0x00},
    ['5'] = {0x7f,0x60,0x7e,0x03,0x03,0x63,0x3e,0x00},
    ['6'] = {0x1e,0x30,0x60,0x7e,0x63,0x63,0x3e,0x00},
    ['7'] = {0x7f,0x63,0x06,0x0c,0x18,0x18,0x18,0x00},
    ['8'] = {0x3e,0x63,0x63,0x3e,0x63,0x63,0x3e,0x00},
    ['9'] = {0x3e,0x63,0x63,0x7f,0x03,0x06,0x3c,0x00},
    [':'] = {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00},
    [';'] = {0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00},
    ['<'] = {0x0c,0x18,0x30,0x60,0x30,0x18,0x0c,0x00},
    ['='] = {0x00,0x7e,0x00,0x7e,0x00,0x00,0x00,0x00},
    ['>'] = {0x30,0x18,0x0c,0x06,0x0c,0x18,0x30,0x00},
    ['?'] = {0x3e,0x63,0x06,0x0c,0x18,0x00,0x18,0x00},
    ['@'] = {0x3e,0x63,0x6f,0x6b,0x6f,0x60,0x3e,0x00},
    ['A'] = {0x18,0x3c,0x66,0x66,0x7e,0x66,0x66,0x00},
    ['B'] = {0x7c,0x66,0x66,0x7c,0x66,0x66,0x7c,0x00},
    ['C'] = {0x3e,0x63,0x60,0x60,0x60,0x63,0x3e,0x00},
    ['D'] = {0x78,0x6c,0x66,0x66,0x66,0x6c,0x78,0x00},
    ['E'] = {0x7f,0x60,0x60,0x78,0x60,0x60,0x7f,0x00},
    ['F'] = {0x7f,0x60,0x60,0x78,0x60,0x60,0x60,0x00},
    ['G'] = {0x3e,0x63,0x60,0x6f,0x63,0x63,0x3e,0x00},
    ['H'] = {0x66,0x66,0x66,0x7e,0x66,0x66,0x66,0x00},
    ['I'] = {0x3e,0x0c,0x0c,0x0c,0x0c,0x0c,0x3e,0x00},
    ['J'] = {0x1f,0x06,0x06,0x06,0x06,0x66,0x3c,0x00},
    ['K'] = {0x66,0x6c,0x78,0x70,0x78,0x6c,0x66,0x00},
    ['L'] = {0x60,0x60,0x60,0x60,0x60,0x60,0x7f,0x00},
    ['M'] = {0x63,0x77,0x7f,0x6b,0x63,0x63,0x63,0x00},
    ['N'] = {0x63,0x73,0x7b,0x6f,0x67,0x53,0x63,0x00},
    ['O'] = {0x3e,0x63,0x63,0x63,0x63,0x63,0x3e,0x00},
    ['P'] = {0x7c,0x66,0x66,0x7c,0x60,0x60,0x60,0x00},
    ['Q'] = {0x3e,0x63,0x63,0x63,0x6b,0x66,0x3d,0x00},
    ['R'] = {0x7c,0x66,0x66,0x7c,0x78,0x6c,0x66,0x00},
    ['S'] = {0x3e,0x63,0x38,0x0e,0x07,0x63,0x3e,0x00},
    ['T'] = {0x7f,0x49,0x0c,0x0c,0x0c,0x0c,0x0c,0x00},
    ['U'] = {0x66,0x66,0x66,0x66,0x66,0x66,0x3e,0x00},
    ['V'] = {0x66,0x66,0x66,0x66,0x66,0x3c,0x18,0x00},
    ['W'] = {0x00,0x00,0x63,0x6b,0x7f,0x36,0x22,0x00},
    ['X'] = {0x00,0x00,0x66,0x3c,0x18,0x3c,0x66,0x00},
    ['Y'] = {0x66,0x66,0x66,0x3c,0x18,0x18,0x18,0x00},
    ['Z'] = {0x7e,0x06,0x0c,0x18,0x30,0x60,0x7e,0x00},
    ['['] = {0x3e,0x30,0x30,0x30,0x30,0x30,0x3e,0x00},
    ['\\']= {0xc0,0x60,0x30,0x18,0x0c,0x06,0x03,0x00},
    [']'] = {0x3e,0x06,0x06,0x06,0x06,0x06,0x3e,0x00},
    ['a'] = {0x00,0x00,0x3c,0x06,0x3e,0x66,0x3b,0x00},
    ['b'] = {0x60,0x60,0x7c,0x66,0x66,0x66,0x7c,0x00},
    ['c'] = {0x00,0x00,0x3c,0x62,0x60,0x62,0x3c,0x00},
    ['d'] = {0x06,0x06,0x3e,0x66,0x66,0x66,0x3e,0x00},
    ['e'] = {0x00,0x00,0x3c,0x66,0x7e,0x60,0x3c,0x00},
    ['f'] = {0x1c,0x22,0x20,0x78,0x20,0x20,0x20,0x00},
    ['g'] = {0x00,0x00,0x3b,0x66,0x66,0x3e,0x06,0x3c},
    ['h'] = {0x60,0x60,0x7c,0x66,0x66,0x66,0x66,0x00},
    ['i'] = {0x18,0x00,0x38,0x18,0x18,0x18,0x3c,0x00},
    ['j'] = {0x06,0x00,0x0e,0x06,0x06,0x06,0x06,0x3c},
    ['k'] = {0x60,0x60,0x6c,0x78,0x70,0x6c,0x66,0x00},
    ['l'] = {0x38,0x18,0x18,0x18,0x18,0x18,0x3c,0x00},
    ['m'] = {0x00,0x00,0x6e,0x7f,0x6b,0x63,0x63,0x00},
    ['n'] = {0x00,0x00,0x7c,0x66,0x66,0x66,0x66,0x00},
    ['o'] = {0x00,0x00,0x3c,0x66,0x66,0x66,0x3c,0x00},
    ['p'] = {0x00,0x00,0x7c,0x66,0x66,0x7c,0x60,0x60},
    ['q'] = {0x00,0x00,0x3e,0x66,0x66,0x3e,0x06,0x06},
    ['r'] = {0x00,0x00,0x5c,0x62,0x60,0x60,0x60,0x00},
    ['s'] = {0x00,0x00,0x3e,0x60,0x3c,0x02,0x7c,0x00},
    ['t'] = {0x20,0x20,0x78,0x20,0x20,0x22,0x1c,0x00},
    ['u'] = {0x00,0x00,0x66,0x66,0x66,0x66,0x3b,0x00},
    ['v'] = {0x00,0x00,0x66,0x66,0x66,0x3c,0x18,0x00},
    ['w'] = {0x00,0x00,0x63,0x6b,0x7f,0x36,0x22,0x00},
    ['x'] = {0x00,0x00,0x66,0x3c,0x18,0x3c,0x66,0x00},
    ['y'] = {0x00,0x00,0x66,0x66,0x66,0x3e,0x06,0x3c},
    ['z'] = {0x00,0x00,0x7e,0x0c,0x18,0x30,0x7e,0x00},
};

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void outb(uint16_t port, uint8_t data) {
    asm volatile ("outb %0, %1" :: "a"(data), "Nd"(port));
}
static inline void outw(uint16_t port, uint16_t data) {
    asm volatile ("outw %0, %1" :: "a"(data), "Nd"(port));
}

void set_video_mode(uint16_t width, uint16_t height, uint16_t bpp) {
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ENABLE);
    outw(VBE_DISPI_IOPORT_DATA, VBE_DISPI_DISABLED);
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_XRES);
    outw(VBE_DISPI_IOPORT_DATA, width);
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_YRES);
    outw(VBE_DISPI_IOPORT_DATA, height);
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_BPP);
    outw(VBE_DISPI_IOPORT_DATA, bpp);
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ENABLE);
    outw(VBE_DISPI_IOPORT_DATA, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
    screen_width = width; screen_height = height; bytes_per_pixel = bpp / 8;
}

void put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < screen_width && y >= 0 && y < screen_height) {
        back_buffer[y * screen_width + x] = color;
    }
}

void blit_bounding_box(int sx, int sy, int w, int h) {
    if (sx < 0) { w += sx; sx = 0; }
    if (sy < 0) { h += sy; sy = 0; }
    if (sx + w > screen_width) w = screen_width - sx;
    if (sy + h > screen_height) h = screen_height - sy;
    if (w <= 0 || h <= 0) return;

    for (int y = sy; y < sy + h; y++) {
        uint32_t* dest = &framebuffer[y * screen_width + sx];
        uint32_t* src = &back_buffer[y * screen_width + sx];
        int count = w;
        asm volatile (
            "cld\n\t"
            "rep movsl"
            : "+D"(dest), "+S"(src), "+c"(count)
            : : "memory"
        );
    }
}

void blit_entire_screen() {
    int total_pixels = screen_width * screen_height;
    uint32_t* dest = framebuffer;
    uint32_t* src = back_buffer;
    asm volatile (
        "cld\n\t"
        "rep movsl"
        : "+D"(dest), "+S"(src), "+c"(total_pixels)
        : : "memory"
    );
}

void draw_rect(int start_x, int start_y, int width, int height, uint32_t color) {
    if (start_x < 0) { width += start_x; start_x = 0; }
    if (start_y < 0) { height += start_y; start_y = 0; }
    if (start_x + width > screen_width) width = screen_width - start_x;
    if (start_y + height > screen_height) height = screen_height - start_y;
    if (width <= 0 || height <= 0) return;

    for (int y = start_y; y < start_y + height; y++) {
        uint32_t* dest = &back_buffer[y * screen_width + start_x];
        int count = width;
        asm volatile (
            "cld\n\t"
            "rep stosl"
            : "+D"(dest), "+c"(count)
            : "a"(color)
            : "memory"
        );
    }
}

void draw_text_absolute(int x_pos, int y_pos, const char* str, uint32_t fg, uint32_t bg) {
    while (*str) {
        unsigned char idx = (unsigned char)*str++;
        for (int y = 0; y < 8; y++) {
            unsigned char row_bits = font8x8[idx][y];
            int TargetY1 = y_pos + (y * 2);
            int TargetY2 = TargetY1 + 1;

            if (TargetY1 >= 0 && TargetY1 < screen_height) {
                uint32_t* line1 = &back_buffer[TargetY1 * screen_width];
                for (int x = 0; x < 8; x++) {
                    int tx = x_pos + x;
                    if (tx >= 0 && tx < screen_width) {
                        if (row_bits & (1 << (7 - x)))      line1[tx] = fg;
                        else if (bg != 0xFFFFFFFF)          line1[tx] = bg;
                    }
                }
            }
            if (TargetY2 >= 0 && TargetY2 < screen_height) {
                uint32_t* line2 = &back_buffer[TargetY2 * screen_width];
                for (int x = 0; x < 8; x++) {
                    int tx = x_pos + x;
                    if (tx >= 0 && tx < screen_width) {
                        if (row_bits & (1 << (7 - x)))      line2[tx] = fg;
                        else if (bg != 0xFFFFFFFF)          line2[tx] = bg;
                    }
                }
            }
        }
        x_pos += 8;
    }
}

void draw_windows() {
    if (window_open) {
        draw_rect(window_x, window_y, window_width, window_height, COLOR_BLUE);
        draw_rect(window_x, window_y, window_width, 22, COLOR_TITLE_BLUE);
        draw_rect(window_x, window_y + 22, window_width, 1, COLOR_WHITE);

        draw_text_absolute(window_x + 6, window_y + 4, "Terminal / Pong Game", COLOR_WHITE, COLOR_TITLE_BLUE);

        draw_rect(window_x + window_width - 18, window_y + 3, 14, 14, COLOR_RED);
        draw_text_absolute(window_x + window_width - 14, window_y + 3, "x", COLOR_WHITE, COLOR_RED);

        if (shell_buf_pos >= 0) {
            int render_start_x = window_x + 8;
            int render_start_y = window_y + 30;
            for (int i = 0; i < terminal_history_count; i++) {
                draw_text_absolute(render_start_x, render_start_y + (i * 16), terminal_history[i], COLOR_WHITE, COLOR_BLUE);
            }
            int cur_y = render_start_y + (terminal_history_count * 16);
            if (cur_y < window_y + window_height - 16) {
                draw_text_absolute(render_start_x, cur_y, "TuxOS> ", COLOR_WHITE, COLOR_BLUE);
                if (shell_buf_pos > 0) {
                    shell_input_buf[shell_buf_pos] = '\0';
                    draw_text_absolute(render_start_x + (7 * 8), cur_y, shell_input_buf, COLOR_WHITE, COLOR_BLUE);
                }
            }
        }
    }

    if (about_open) {
        draw_rect(about_x, about_y, about_width, about_height, COLOR_LIGHT_GRAY);
        draw_rect(about_x, about_y, about_width, 22, COLOR_TITLE_BLUE);
        draw_rect(about_x, about_y + 22, about_width, 1, COLOR_WHITE);
        draw_text_absolute(about_x + 6, about_y + 4, "About System", COLOR_WHITE, COLOR_TITLE_BLUE);

        draw_rect(about_x + about_width - 18, about_y + 3, 14, 14, COLOR_RED);
        draw_text_absolute(about_x + about_width - 14, about_y + 3, "x", COLOR_WHITE, COLOR_RED);

        draw_text_absolute(about_x + 20, about_y + 45, "TuxOS v0.2.4", COLOR_BLACK, COLOR_LIGHT_GRAY);
        draw_text_absolute(about_x + 20, about_y + 70, "Hobby 32-bit Kernel stack", COLOR_DARK_GRAY, COLOR_LIGHT_GRAY);
        draw_text_absolute(about_x + 20, about_y + 95, "Architecture: x86 (i386)", COLOR_DARK_GRAY, COLOR_LIGHT_GRAY);
    }
}

void draw_desktop_background() {
    draw_rect(0, 28, screen_width, screen_height - 28, COLOR_DESKTOP);
}

void update_ui_chrome() {
    draw_rect(0, 0, screen_width, 28, COLOR_LIGHT_GRAY);
    draw_rect(0, 27, screen_width, 1, COLOR_DARK_GRAY);

    draw_text_absolute(12, 6, "Apps      About", COLOR_BLACK, COLOR_LIGHT_GRAY);
    draw_text_absolute(670, 6, "TuxOS v0.2.4", COLOR_BLACK, COLOR_LIGHT_GRAY);

    if (menu_open) {
        draw_rect(6, 28, 90, 52, COLOR_LIGHT_GRAY);
        draw_rect(6, 28, 1, 52, COLOR_DARK_GRAY);
        draw_rect(95, 28, 1, 52, COLOR_DARK_GRAY);
        draw_rect(6, 80, 90, 1, COLOR_DARK_GRAY);
        draw_text_absolute(12, 34, "Terminal", COLOR_BLACK, COLOR_LIGHT_GRAY);
        draw_text_absolute(12, 56, "Pong Game", COLOR_BLACK, COLOR_LIGHT_GRAY);
    }
}

void redraw_all() {
    draw_desktop_background();
    draw_windows();
    update_ui_chrome();
}

void append_history(const char* line) {
    if (terminal_history_count >= MAX_ROWS) {
        for (int i = 1; i < MAX_ROWS; i++) {
            int j = 0;
            while (terminal_history[i][j] && j < MAX_COLS) {
                terminal_history[i-1][j] = terminal_history[i][j];
                j++;
            }
            terminal_history[i-1][j] = '\0';
        }
        terminal_history_count = MAX_ROWS - 1;
    }

    int k = 0;
    while (line[k] && k < MAX_COLS) {
        terminal_history[terminal_history_count][k] = line[k];
        k++;
    }
    terminal_history[terminal_history_count][k] = '\0';
    terminal_history_count++;
}

void draw_mouse_pointer_front(int mx, int my, uint32_t color) {
    static const uint8_t cursor_map[8] = {
        0b10000000, 0b11000000, 0b11100000, 0b11110000,
        0b11111000, 0b11110000, 0b10110000, 0b00011000
    };
    for(int y=0; y<8; y++) {
        for(int x=0; x<8; x++) {
            if(cursor_map[y] & (1 << (7-x))) {
                int tx = mx + x;
                int ty = my + y;
                if (tx >= 0 && tx < screen_width && ty >= 0 && ty < screen_height) {
                    framebuffer[ty * screen_width + tx] = color;
                }
            }
        }
    }
}

void update_mouse_position() {
    blit_bounding_box(old_mouse_x, old_mouse_y, 8, 8);
    old_mouse_x = mouse_x;
    old_mouse_y = mouse_y;
    draw_mouse_pointer_front(mouse_x, mouse_y, COLOR_RED);
}

void mouse_wait(uint8_t a_type) {
    uint32_t timeout = 100000;
    if (a_type == 0) { while (timeout-- && (inb(0x64) & 1) == 0); }
    else { while (timeout-- && (inb(0x64) & 2)); }
}

void mouse_write(uint8_t a_write) {
    mouse_wait(1); outb(0x64, 0xD4);
    mouse_wait(1); outb(0x60, a_write);
}

void init_mouse() {
    mouse_wait(1); outb(0x64, 0xA8);
    mouse_wait(1); outb(0x64, 0x20);
    mouse_wait(0); uint8_t status = (inb(0x60) | 2);
    mouse_wait(1); outb(0x64, 0x60);
    mouse_wait(1); outb(0x60, status);
    mouse_write(0xF6); inb(0x60);
    mouse_write(0xF4); inb(0x60);
}

void handle_mouse_polling() {
    if ((inb(0x64) & 1) && (inb(0x64) & 0x20)) {
        uint8_t data = inb(0x60);
        switch(mouse_cycle) {
            case 0:
                mouse_byte[0] = data;
                if (!(data & 0x08)) return;
                mouse_cycle++;
                break;
            case 1: mouse_byte[1] = data; mouse_cycle++; break;
            case 2:
                mouse_byte[2] = data;
                mouse_cycle = 0;

                int rel_x = mouse_byte[1]; int rel_y = mouse_byte[2];
                if (mouse_byte[0] & 0x10) rel_x |= 0xFFFFFF00;
                if (mouse_byte[0] & 0x20) rel_y |= 0xFFFFFF00;

                mouse_x += (rel_x * 3) / 2;
                mouse_y -= (rel_y * 3) / 2;

                if (mouse_x < 0) mouse_x = 0; if (mouse_y < 0) mouse_y = 0;
                if (mouse_x > screen_width - 8) mouse_x = screen_width - 8;
                if (mouse_y > screen_height - 8) mouse_y = screen_height - 8;

                uint8_t click = mouse_byte[0] & 1;
                if (click && !(prev_mouse_buttons & 1)) {
                    if (mouse_y >= 0 && mouse_y <= 28) {
                        if (mouse_x >= 10 && mouse_x <= 65) {
                            menu_open = !menu_open;
                            redraw_all();
                            blit_entire_screen();
                        }
                        else if (mouse_x >= 84 && mouse_x <= 140) {
                            about_open = 1;
                            if(menu_open) menu_open = 0;
                            redraw_all();
                            blit_entire_screen();
                        } else {
                            if(menu_open) { menu_open = 0; redraw_all(); blit_entire_screen(); }
                        }
                    }
                    else if (menu_open && mouse_x >= 6 && mouse_x <= 96) {
                        if (mouse_y >= 28 && mouse_y <= 54) {
                            window_open = 1;
                            menu_open = 0;
                            redraw_all();
                            blit_entire_screen();
                        }
                        else if (mouse_y > 54 && mouse_y <= 80) {
                            menu_open = 0;
                            window_open = 1;
                            redraw_all();
                            blit_entire_screen();
                            run_pong_game();
                        }
                    }
                    else if (about_open && mouse_y >= about_y + 3 && mouse_y <= about_y + 17 &&
                               mouse_x >= about_x + about_width - 18 && mouse_x <= about_x + about_width - 4) {
                        about_open = 0;
                        redraw_all();
                        blit_entire_screen();
                    }
                    else if (window_open && mouse_y >= window_y + 3 && mouse_y <= window_y + 17 &&
                               mouse_x >= window_x + window_width - 18 && mouse_x <= window_x + window_width - 4) {
                        window_open = 0;
                        redraw_all();
                        blit_entire_screen();
                    }
                    else if (about_open && mouse_y >= about_y && mouse_y <= about_y + 22 &&
                               mouse_x >= about_x && mouse_x <= about_x + about_width - 20) {
                        is_dragging = 2;
                        drag_offset_x = mouse_x - about_x;
                        drag_offset_y = mouse_y - about_y;
                    }
                    else if (window_open && mouse_y >= window_y && mouse_y <= window_y + 22 &&
                               mouse_x >= window_x && mouse_x <= window_x + window_width - 20) {
                        is_dragging = 1;
                        drag_offset_x = mouse_x - window_x;
                        drag_offset_y = mouse_y - window_y;
                    } else {
                        if(menu_open) { menu_open = 0; redraw_all(); blit_entire_screen(); }
                    }
                }

                if (!click) is_dragging = 0;

                if (is_dragging == 1) {
                    int old_wx = window_x; int old_wy = window_y;
                    window_x = mouse_x - drag_offset_x;
                    window_y = mouse_y - drag_offset_y;
                    if (window_y < 28) window_y = 28;

                    if (old_wx != window_x || old_wy != window_y) {
                        draw_rect(old_wx, old_wy, window_width, window_height, COLOR_DESKTOP);
                        draw_windows();
                        blit_bounding_box(old_wx, old_wy, window_width, window_height);
                        blit_bounding_box(window_x, window_y, window_width, window_height);
                    }
                }
                else if (is_dragging == 2) {
                    int old_ax = about_x; int old_ay = about_y;
                    about_x = mouse_x - drag_offset_x;
                    about_y = mouse_y - drag_offset_y;
                    if (about_y < 28) about_y = 28;

                    if (old_ax != about_x || old_ay != about_y) {
                        draw_rect(old_ax, old_ay, about_width, about_height, COLOR_DESKTOP);
                        draw_windows();
                        blit_bounding_box(old_ax, old_ay, about_width, about_height);
                        blit_bounding_box(about_x, about_y, about_width, about_height);
                    }
                }

                prev_mouse_buttons = mouse_byte[0];
                update_mouse_position();
                break;
        }
    }
}

static int kbhit() {
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    return (status & 0x01) && !(status & 0x20);
}

static const char scancode_lower[] = {
    0,0, '1','2','3','4','5','6','7','8','9','0','-','=',0,
    0,'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,
    '\\','z','x','c','v','b','n','m',',','.','/',0,0,0,' '
};
static const char scancode_upper[] = {
    0,0, '!','@','#','$','%','^','&','*','(',')','_','+',0,
    0,'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',':','"','~',0,
    '|','Z','X','C','V','B','N','M','<','>','?',0,0,0,' '
};

void handle_keyboard_input() {
    if (kbhit()) {
        unsigned char sc = inb(KEYBOARD_DATA_PORT);
        if (sc & 0x80) {
            sc &= 0x7F;
            if (sc == 0x2A || sc == 0x36) shell_shift_state = 0;
            return;
        }
        if (sc == 0x2A || sc == 0x36) { shell_shift_state = 1; return; }

        if (!window_open) return;

        if (sc == 0x0E) {
            if (shell_buf_pos > 0) {
                shell_buf_pos--;
                shell_input_buf[shell_buf_pos] = '\0';
                draw_windows();
                blit_bounding_box(window_x, window_y, window_width, window_height);
            }
        } else if (sc == 0x1C) {
            shell_input_buf[shell_buf_pos] = '\0';

            char logged_prompt[128] = "TuxOS> ";
            int lp = 7;
            for(int i = 0; i < shell_buf_pos && lp < 60; i++) {
                logged_prompt[lp++] = shell_input_buf[i];
            }
            logged_prompt[lp] = '\0';
            append_history(logged_prompt);

            char *cmd = shell_input_buf, *args = "";
            for (int i = 0; i < shell_buf_pos; i++) {
                if (shell_input_buf[i] == ' ') {
                    shell_input_buf[i] = 0;
                    args = shell_input_buf + i + 1;
                    break;
                }
            }

            if (shell_buf_pos > 0) {
                shell_execute(cmd, args);
            }

            shell_buf_pos = 0;
            draw_windows();
            blit_bounding_box(window_x, window_y, window_width, window_height);
        } else if (sc < sizeof(scancode_lower)) {
            char ascii = shell_shift_state ? scancode_upper[sc] : scancode_lower[sc];
            if (ascii && shell_buf_pos < 45) {
                shell_input_buf[shell_buf_pos++] = ascii;
                draw_windows();
                blit_bounding_box(window_x, window_y, window_width, window_height);
            }
        }
    }
}

int atoi(const char* s) {
    int res = 0, sign = 1;
    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res * sign;
}

void itoa(int n, char* s) {
    int i = 0, is_neg = 0;
    if (n < 0) { is_neg = 1; n = -n; }
    do {
        s[i++] = '0' + (n % 10);
        n /= 10;
    } while (n > 0);
    if (is_neg) s[i++] = '-';
    s[i] = '\0';
    for (int j = 0; j < i / 2; j++) {
        char tmp = s[j];
        s[j] = s[i - 1 - j];
        s[i - 1 - j] = tmp;
    }
}

uint32_t parse_hex(const char* str) {
    uint32_t val = 0;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) str += 2;
    while (*str) {
        char c = *str++;
        val <<= 4;
        if (c >= '0' && c <= '9') val += (c - '0');
        else if (c >= 'a' && c <= 'f') val += (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') val += (c - 'A' + 10);
        else return val >> 4;
    }
    return val;
}

void run_pong_game() {
    int canvas_x = window_x + 8;
    int canvas_y = window_y + 30;
    int canvas_w = window_width - 16;
    int canvas_h = window_height - 48;

    int paddle_w = 8;
    int paddle_h = 42;
    int player_y = canvas_y + (canvas_h / 2) - (paddle_h / 2);
    int ai_y = canvas_y + (canvas_h / 2) - (paddle_h / 2);

    int ball_x = canvas_x + (canvas_w / 2);
    int ball_y = canvas_y + (canvas_h / 2);
    int ball_dx = 4;
    int ball_dy = 2;

    int player_score = 0;
    int ai_score = 0;

    int key_w_pressed = 0;
    int key_s_pressed = 0;

    int old_player_y = player_y;
    int old_ai_y = ai_y;
    int old_ball_x = ball_x;
    int old_ball_y = ball_y;

    draw_rect(window_x, window_y, window_width, window_height, COLOR_BLUE);
    draw_rect(window_x, window_y, window_width, 22, COLOR_TITLE_BLUE);
    draw_rect(window_x, window_y + 22, window_width, 1, COLOR_WHITE);
    draw_text_absolute(window_x + 6, window_y + 4, "Terminal / Pong Game", COLOR_WHITE, COLOR_TITLE_BLUE);
    draw_rect(window_x + window_width - 18, window_y + 3, 14, 14, COLOR_RED);
    draw_text_absolute(window_x + window_width - 14, window_y + 3, "x", COLOR_WHITE, COLOR_RED);

    draw_rect(canvas_x, canvas_y, canvas_w, canvas_h, COLOR_BLACK);

    while (1) {
        handle_mouse_polling();

        if (!window_open) {
            goto exit_pong;
        }

        while (kbhit()) {
            unsigned char sc = inb(KEYBOARD_DATA_PORT);
            unsigned char is_break = (sc & 0x80);
            unsigned char actual_sc = sc & 0x7F;

            if (actual_sc == 0x11) {       // 'W'
                key_w_pressed = !is_break;
            } else if (actual_sc == 0x1F) { // 'S'
                key_s_pressed = !is_break;
            } else if (actual_sc == 0x10 && !is_break) { // 'Q' to Quit
                goto exit_pong;
            }
        }

        old_player_y = player_y;
        old_ai_y = ai_y;
        old_ball_x = ball_x;
        old_ball_y = ball_y;

        if (key_w_pressed) {
            player_y -= 4;
            if (player_y < canvas_y) player_y = canvas_y;
        }
        if (key_s_pressed) {
            player_y += 4;
            if (player_y + paddle_h > canvas_y + canvas_h) player_y = canvas_y + canvas_h - paddle_h;
        }

        if (ball_y < ai_y + (paddle_h / 2) - 4) {
            ai_y -= 2;
            if (ai_y < canvas_y) ai_y = canvas_y;
        } else if (ball_y > ai_y + (paddle_h / 2) + 4) {
            ai_y += 2;
            if (ai_y + paddle_h > canvas_y + canvas_h) ai_y = canvas_y + canvas_h - paddle_h;
        }

        ball_x += ball_dx;
        ball_y += ball_dy;

        if (ball_y <= canvas_y) {
            ball_y = canvas_y;
            ball_dy = -ball_dy;
        }
        else if (ball_y >= canvas_y + canvas_h - 6) {
            ball_y = canvas_y + canvas_h - 6;
            ball_dy = -ball_dy;
        }

        if (ball_x <= canvas_x + paddle_w) {
            if (ball_y + 6 >= player_y && ball_y <= player_y + paddle_h) {
                ball_x = canvas_x + paddle_w;
                ball_dx = -ball_dx;
                random_seed = (random_seed * 1103515245 + 12345) & 0x7FFFFFFF;
                ball_dy += (random_seed % 3) - 1;
                if (ball_dy > 5) ball_dy = 5;
                if (ball_dy < -5) ball_dy = -5;
            } else {
                ai_score++;
                draw_rect(canvas_x, canvas_y, canvas_w, canvas_h, COLOR_BLACK);
                ball_x = canvas_x + (canvas_w / 2);
                ball_y = canvas_y + (canvas_h / 2);
                ball_dx = 4; ball_dy = 2;
            }
        }

        if (ball_x >= canvas_x + canvas_w - paddle_w - 6) {
            if (ball_y + 6 >= ai_y && ball_y <= ai_y + paddle_h) {
                ball_x = canvas_x + canvas_w - paddle_w - 6;
                ball_dx = -ball_dx;
            } else {
                player_score++;
                draw_rect(canvas_x, canvas_y, canvas_w, canvas_h, COLOR_BLACK);
                ball_x = canvas_x + (canvas_w / 2);
                ball_y = canvas_y + (canvas_h / 2);
                ball_dx = -4; ball_dy = -2;
            }
        }

        draw_rect(canvas_x, old_player_y, paddle_w, paddle_h, COLOR_BLACK);
        draw_rect(canvas_x + canvas_w - paddle_w, old_ai_y, paddle_w, paddle_h, COLOR_BLACK);
        draw_rect(old_ball_x, old_ball_y, 6, 6, COLOR_BLACK);

        for (int i = canvas_y; i < canvas_y + canvas_h; i += 16) {
            draw_rect(canvas_x + (canvas_w / 2) - 1, i, 2, 8, COLOR_DARK_GRAY);
        }

        draw_rect(canvas_x, player_y, paddle_w, paddle_h, COLOR_WHITE);
        draw_rect(canvas_x + canvas_w - paddle_w, ai_y, paddle_w, paddle_h, COLOR_WHITE);
        draw_rect(ball_x, ball_y, 6, 6, COLOR_RED);

        char s_buf[16];
        itoa(player_score, s_buf);
        draw_text_absolute(canvas_x + (canvas_w / 4), canvas_y + 12, s_buf, COLOR_WHITE, COLOR_BLACK);
        itoa(ai_score, s_buf);
        draw_text_absolute(canvas_x + (3 * canvas_w / 4), canvas_y + 12, s_buf, COLOR_WHITE, COLOR_BLACK);

        blit_bounding_box(window_x, window_y, window_width, window_height);

        for (volatile int d = 0; d < 40000; d++);

        if (player_score >= 5 || ai_score >= 5) break;
    }

exit_pong:
    terminal_history_count = 0;
    if (player_score >= 5) {
        append_history("Game Over: Player Wins!");
    } else if (ai_score >= 5) {
        append_history("Game Over: AI Wins!");
    } else {
        append_history("Pong match closed.");
    }
    redraw_all();
    blit_entire_screen();
}

void shell_execute(const char* cmd, const char* args) {
    if (!strcmp(cmd, "help")) {
        append_history("Commands: help, uname, clear, rand, time, shutdown");
        append_history("          echo, calc, mdump, game, pong");
    } else if (!strcmp(cmd, "uname")) {
        append_history("TuxOS v0.2.4");
    } else if (!strcmp(cmd, "clear")) {
        terminal_history_count = 0;
    } else if (!strcmp(cmd, "echo")) {
        if (strlen(args) > 0) append_history(args);
        else append_history("");
    } else if (!strcmp(cmd, "rand")) {
        random_seed = (random_seed * 1103515245 + 12345) & 0x7FFFFFFF;
        int num = random_seed % 100;
        char buf[32] = "Random generated: ";
        char dig[8]; itoa(num, dig);
        int idx = 18, d = 0;
        while(dig[d]) buf[idx++] = dig[d++];
        buf[idx] = '\0';
        append_history(buf);
    } else if (!strcmp(cmd, "time")) {
        boot_epoch += 45;
        char t_buf[64] = "Ticks since boot: ";
        int base_len = 18;
        char digits[16]; itoa(boot_epoch, digits);
        int d_idx = 0;
        while(digits[d_idx]) t_buf[base_len++] = digits[d_idx++];
        t_buf[base_len] = '\0';
        append_history(t_buf);
    } else if (!strcmp(cmd, "calc")) {
        if (strlen(args) == 0) { append_history("Usage: calc [num1] [op] [num2]"); return; }
        char n1_buf[16], op_buf[4], n2_buf[16];
        int i = 0, p = 0;
        while(args[i] && args[i] != ' ' && p < 15) n1_buf[p++] = args[i++]; n1_buf[p] = 0;
        if(args[i] == ' ') i++;
        p = 0;
        while(args[i] && args[i] != ' ' && p < 3) op_buf[p++] = args[i++]; op_buf[p] = 0;
        if(args[i] == ' ') i++;
        p = 0;
        while(args[i] && args[i] != ' ' && p < 15) n2_buf[p++] = args[i++]; n2_buf[p] = 0;

        int n1 = atoi(n1_buf), n2 = atoi(n2_buf), ans = 0, bad = 0;
        if (!strcmp(op_buf, "+")) ans = n1 + n2;
        else if (!strcmp(op_buf, "-")) ans = n1 - n2;
        else if (!strcmp(op_buf, "*")) ans = n1 * n2;
        else if (!strcmp(op_buf, "/")) { if(n2 == 0) bad = 1; else ans = n1 / n2; }
        else bad = 2;

        if (bad == 1) append_history("Error: Divide by Zero.");
        else if (bad == 2) append_history("Error: Unknown operator.");
        else {
            char out[32] = "Result: "; char dig[16]; itoa(ans, dig);
            int o_idx = 8, d_idx = 0;
            while(dig[d_idx]) out[o_idx++] = dig[d_idx++]; out[o_idx] = 0;
            append_history(out);
        }
    } else if (!strcmp(cmd, "mdump")) {
        if (strlen(args) == 0) { append_history("Usage: mdump [hex_addr]"); return; }
        uint8_t* ptr = (uint8_t*)parse_hex(args);
        char hex_out[64] = "Data: ";
        int h_idx = 6;
        for (int x = 0; x < 8; x++) {
            uint8_t val = ptr[x];
            char hi = (val >> 4) & 0xF; char lo = val & 0xF;
            hex_out[h_idx++] = (hi < 10) ? ('0' + hi) : ('A' + hi - 10);
            hex_out[h_idx++] = (lo < 10) ? ('0' + lo) : ('A' + lo - 10);
            hex_out[h_idx++] = ' ';
        }
        hex_out[h_idx] = '\0';
        append_history(hex_out);
    } else if (!strcmp(cmd, "game")) {
        if (strlen(args) == 0) {
            random_seed = (random_seed * 1103515245 + 12345) & 0x7FFFFFFF;
            target_game_num = random_seed % 20;
            append_history("Guessing match loaded! Range: 0-19.");
            append_history("Type: game [number]");
        } else {
            if (target_game_num == -1) { append_history("Run 'game' without arguments first."); return; }
            int guess = atoi(args);
            if (guess < target_game_num) append_history("Too low!");
            else if (guess > target_game_num) append_history("Too high!");
            else { append_history("Correct! You won!"); target_game_num = -1; }
        }
    } else if (!strcmp(cmd, "pong")) {
        run_pong_game();
    } else if (!strcmp(cmd, "shutdown")) {
        append_history("Shutting down kernel stack cleanly...");
        outw(0xB004, 0x2000);
        outw(0x604, 0x2000);
    } else if (strlen(cmd) > 0) {
        append_history("Unknown command.");
    }
}

int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a - b[0];
}
int strlen(const char *s) { int l = 0; while(*s++) l++; return l; }

void kernel_main(uint32_t* vram) {
    set_video_mode(800, 600, 32);
    framebuffer = ((uint32_t)vram < 0x00100000) ? (uint32_t*)0xFD000000 : vram;

    append_history("Type 'help' for instructions.");

    init_mouse();
    redraw_all();
    blit_entire_screen();

    draw_mouse_pointer_front(mouse_x, mouse_y, COLOR_RED);

    while (1) {
        handle_mouse_polling();
        handle_keyboard_input();
    }
}
