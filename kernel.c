/* kernel.c - TuxOS 0.2.3 – Lean Monolithic Stack */

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

#define VBE_DISPI_IOPORT_INDEX 0x01CE
#define VBE_DISPI_IOPORT_DATA  0x01CF

#define VBE_DISPI_INDEX_XRES        1
#define VBE_DISPI_INDEX_YRES        2
#define VBE_DISPI_INDEX_BPP         3
#define VBE_DISPI_INDEX_ENABLE      4

#define VBE_DISPI_DISABLED          0x00
#define VBE_DISPI_ENABLED           0x01
#define VBE_DISPI_LFB_ENABLED       0x40

static int screen_width  = 800;
static int screen_height = 600;
static int bytes_per_pixel = 4;
static int screen_pitch  = 3200;

#define MAX_ROWS 37
#define MAX_COLS 100

void put_pixel(int x, int y, uint32_t color);
void draw_char_gfx(int col, int row, char c, uint32_t fg, uint32_t bg);
void clear_screen();
void print_char(char c);
void print_string(const char *str);
static int get_scancode();
static int kbhit();
void int_to_str(int num, char *buf);
int rand_range(int min, int max);
int strlen(const char *s);

#define COLOR_WHITE      0xFFFFFF
#define COLOR_BLACK      0x000000
#define COLOR_BLUE       0x1A1B26
#define COLOR_GREEN      0x00FF00
#define COLOR_RED        0xFF0000

#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_STATUS_PORT 0x64

static uint8_t* framebuffer = 0;
static int cursor_row = 0;
static int cursor_col = 0;
static unsigned int boot_epoch = 0;
static unsigned int random_seed = 0;

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
    ['W'] = {0x63,0x63,0x63,0x6b,0x7f,0x77,0x63,0x00},
    ['X'] = {0x66,0x66,0x3c,0x18,0x3c,0x66,0x66,0x00},
    ['Y'] = {0x66,0x66,0x66,0x3c,0x0c,0x0c,0x0c,0x00},
    ['Z'] = {0x7f,0x46,0x0c,0x18,0x30,0x61,0x7f,0x00},
    ['a'] = {0x00,0x00,0x3e,0x03,0x3f,0x63,0x3d,0x00},
    ['b'] = {0x60,0x60,0x7c,0x66,0x66,0x66,0x7c,0x00},
    ['c'] = {0x00,0x00,0x3e,0x60,0x60,0x63,0x3e,0x00},
    ['d'] = {0x03,0x03,0x3f,0x63,0x63,0x63,0x3d,0x00},
    ['e'] = {0x00,0x00,0x3e,0x63,0x7f,0x60,0x3e,0x00},
    ['f'] = {0x1c,0x36,0x30,0x78,0x30,0x30,0x30,0x00},
    ['g'] = {0x00,0x00,0x3d,0x63,0x63,0x3f,0x03,0x3e},
    ['h'] = {0x60,0x60,0x7c,0x66,0x66,0x66,0x66,0x00},
    ['i'] = {0x0c,0x00,0x1c,0x0c,0x0c,0x0c,0x1e,0x00},
    ['j'] = {0x06,0x00,0x0e,0x06,0x06,0x66,0x66,0x3c},
    ['k'] = {0x60,0x60,0x66,0x6c,0x78,0x6c,0x66,0x00},
    ['l'] = {0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e,0x00},
    ['m'] = {0x00,0x00,0x6d,0x7f,0x6b,0x63,0x63,0x00},
    ['n'] = {0x00,0x00,0x7c,0x66,0x66,0x66,0x66,0x00},
    ['o'] = {0x00,0x00,0x3e,0x63,0x63,0x63,0x3e,0x00},
    ['p'] = {0x00,0x00,0x7c,0x66,0x66,0x7c,0x60,0x60},
    ['q'] = {0x00,0x00,0x3d,0x63,0x63,0x3f,0x03,0x03},
    ['r'] = {0x00,0x00,0x7c,0x66,0x60,0x60,0x60,0x00},
    ['s'] = {0x00,0x00,0x3e,0x60,0x3e,0x03,0x3e,0x00},
    ['t'] = {0x30,0x30,0x7c,0x30,0x30,0x36,0x1c,0x00},
    ['u'] = {0x00,0x00,0x66,0x66,0x66,0x66,0x3d,0x00},
    ['v'] = {0x00,0x00,0x66,0x66,0x66,0x3c,0x18,0x00},
    ['w'] = {0x00,0x00,0x63,0x63,0x6b,0x7f,0x36,0x00},
    ['x'] = {0x00,0x00,0x66,0x3c,0x18,0x3c,0x66,0x00},
    ['y'] = {0x00,0x00,0x66,0x66,0x66,0x3f,0x03,0x3e},
    ['z'] = {0x00,0x00,0x7f,0x0c,0x18,0x30,0x7f,0x00},
    ['['] = {0x3e,0x30,0x30,0x30,0x30,0x30,0x3e,0x00},
    ['\\']= {0xc0,0x60,0x30,0x18,0x0c,0x06,0x03,0x00},
    [']'] = {0x3e,0x06,0x06,0x06,0x06,0x06,0x3e,0x00},
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

void write_vbe(uint16_t index, uint16_t value) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA, value);
}

void set_video_mode(uint16_t width, uint16_t height, uint16_t bpp) {
    write_vbe(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    write_vbe(VBE_DISPI_INDEX_XRES, width);
    write_vbe(VBE_DISPI_INDEX_YRES, height);
    write_vbe(VBE_DISPI_INDEX_BPP, bpp);
    write_vbe(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
    
    screen_width = width;
    screen_height = height;
    bytes_per_pixel = bpp / 8;
    screen_pitch = screen_width * bytes_per_pixel;
}

void put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < screen_width && y >= 0 && y < screen_height) {
        int offset = (y * screen_pitch) + (x * bytes_per_pixel);
        framebuffer[offset]     = color & 0xFF;         
        framebuffer[offset + 1] = (color >> 8) & 0xFF;  
        framebuffer[offset + 2] = (color >> 16) & 0xFF; 
        if (bytes_per_pixel == 4) {
            framebuffer[offset + 3] = (color >> 24) & 0xFF; 
        }
    }
}

void draw_char_gfx(int col, int row, char c, uint32_t fg, uint32_t bg) {
    int start_x = col * 8;
    int start_y = row * 16;
    unsigned char idx = (unsigned char)c;
    
    for (int y = 0; y < 8; y++) {
        unsigned char row_bits = font8x8[idx][y];
        for (int x = 0; x < 8; x++) {
            if (row_bits & (1 << (7 - x))) {
                put_pixel(start_x + x, start_y + (y * 2), fg);
                put_pixel(start_x + x, start_y + (y * 2) + 1, fg);
            } else {
                put_pixel(start_x + x, start_y + (y * 2), bg);
                put_pixel(start_x + x, start_y + (y * 2) + 1, bg);
            }
        }
    }
}

void clear_screen() {
    for (int y = 0; y < screen_height; y++) {
        for (int x = 0; x < screen_width; x++) {
            put_pixel(x, y, COLOR_BLUE);
        }
    }
    cursor_row = cursor_col = 0;
}

static void scroll_up() {
    int line_offset_src = 16 * screen_pitch;
    int move_size = (screen_height - 16) * screen_pitch;
    
    for (int i = 0; i < move_size; i++) {
        framebuffer[i] = framebuffer[i + line_offset_src];
    }
    
    for (int y = screen_height - 16; y < screen_height; y++) {
        for (int x = 0; x < screen_width; x++) {
            put_pixel(x, y, COLOR_BLUE);
        }
    }
}

void print_char(char c) {
    if (c == '\n') { 
        cursor_col = 0; 
        cursor_row++; 
    } else if (c == '\r') { 
        cursor_col = 0; 
    } else if (c == '\t') {
        cursor_col = (cursor_col + 4) & ~3;
    } else {
        draw_char_gfx(cursor_col, cursor_row, c, COLOR_WHITE, COLOR_BLUE);
        cursor_col++;
    }
    if (cursor_col >= MAX_COLS) { cursor_col = 0; cursor_row++; }
    if (cursor_row >= MAX_ROWS) { cursor_row = MAX_ROWS - 1; scroll_up(); }
}

void print_string(const char *str) {
    while (*str) print_char(*str++);
}

static void reverse_str(char *s, int len) {
    for (int i=0; i<len/2; i++) { char t=s[i]; s[i]=s[len-1-i]; s[len-1-i]=t; }
}

void int_to_str(int num, char *buf) {
    int i=0, sign=0;
    if (num<0) { sign=1; num=-num; }
    do { buf[i++] = '0' + (num%10); num/=10; } while (num>0);
    if (sign) buf[i++] = '-';
    buf[i] = '\0';
    reverse_str(buf,i);
}

static int cmos_read(unsigned char reg) {
    outb(0x70, (1<<7) | reg);
    for (volatile int i=0; i<1000; i++);
    return inb(0x71);
}

static unsigned char bcd_to_bin(unsigned char bcd) {
    return ((bcd>>4)*10) + (bcd & 0x0F);
}

void read_rtc(unsigned char *hour, unsigned char *min, unsigned char *sec,
              unsigned char *day, unsigned char *month, unsigned char *year) {
    while (cmos_read(0x0A) & 0x80);
    *sec   = bcd_to_bin(cmos_read(0x00));
    *min   = bcd_to_bin(cmos_read(0x02));
    *hour  = bcd_to_bin(cmos_read(0x04));
    *day   = bcd_to_bin(cmos_read(0x07));
    *month = bcd_to_bin(cmos_read(0x08));
    *year  = bcd_to_bin(cmos_read(0x09));
}

unsigned int rtc_to_epoch(unsigned char h, unsigned char m, unsigned char s,
                          unsigned char d, unsigned char mo, unsigned char y) {
    unsigned int days = (y*365) + (mo*30) + d;
    return (days*86400) + (h*3600) + (m*60) + s;
}

unsigned int get_rtc_epoch() {
    unsigned char h,m,s,d,mo,y;
    read_rtc(&h,&m,&s,&d,&mo,&y);
    return rtc_to_epoch(h,m,s,d,mo,y);
}

static void srand(unsigned int seed) { random_seed = seed; }
static int rand() {
    random_seed = (1103515245*random_seed + 12345) & 0x7fffffff;
    return random_seed;
}
int rand_range(int min, int max) {
    if (max <= min) return min;
    return min + (rand() % (max - min + 1));
}

void print_centered(int row, const char *str, uint32_t fg, uint32_t bg) {
    int len = strlen(str);
    int start_col = (100 - len) / 2;
    if (start_col < 0) start_col = 0;
    
    for (int i = 0; i < len; i++) {
        draw_char_gfx(start_col + i, row, str[i], fg, bg);
    }
}

void kernel_panic(const char *reason) {
    for (int y = 0; y < screen_height; y++) {
        for (int x = 0; x < screen_width; x++) {
            put_pixel(x, y, COLOR_RED);
        }
    }
    
    print_centered(14, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!", COLOR_WHITE, COLOR_RED);
    print_centered(16, "KERNEL PANIC :(", COLOR_WHITE, COLOR_RED);
    print_centered(18, reason, COLOR_WHITE, COLOR_RED);
    print_centered(20, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!", COLOR_WHITE, COLOR_RED);
    
    asm volatile ("cli; hlt");
    while(1);
}

static const char scancode_lower[] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,
    0,'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,
    '\\','z','x','c','v','b','n','m',',','.','/',0,0,0,' '
};
static const char scancode_upper[] = {
    0,0,'!','@','#','$','%','^','&','*','(',')','_','+',0,
    0,'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',':','"','~',0,
    '|','Z','X','C','V','B','N','M','<','>','?',0,0,0,' '
};

static int kbhit() { return inb(KEYBOARD_STATUS_PORT) & 0x01; }
static int get_scancode() { if (kbhit()) return inb(KEYBOARD_DATA_PORT); return -1; }

static void flush_keyboard() {
    volatile uint8_t dummy;
    while (kbhit()) { dummy = inb(KEYBOARD_DATA_PORT); }
}

int read_line(char *buffer, int max) {
    int pos=0, shift=0;
    flush_keyboard();
    while (1) {
        while (!(inb(KEYBOARD_STATUS_PORT)&0x01));
        unsigned char sc = inb(KEYBOARD_DATA_PORT);
        if (sc & 0x80) { sc &= 0x7F; if (sc==0x2A||sc==0x36) shift=0; continue; }
        if (sc==0x2A || sc==0x36) { shift=1; continue; }
        if (sc==0x0E) {
            if (pos>0 && cursor_col>0) {
                pos--; cursor_col--;
                draw_char_gfx(cursor_col, cursor_row, ' ', COLOR_WHITE, COLOR_BLUE);
            }
        } else if (sc==0x1C) {
            print_char('\n'); buffer[pos]='\0'; return pos;
        } else if (sc < sizeof(scancode_lower)) {
            char ascii = shift ? scancode_upper[sc] : scancode_lower[sc];
            if (ascii && pos<max-1) { buffer[pos++]=ascii; print_char(ascii); }
        }
    }
}

int strcmp(const char *a, const char *b) {
    while (*a && *a==*b) { a++; b++; }
    return *a - b[0];
}
int strlen(const char *s) { int l=0; while(*s++) l++; return l; }

void print_hex(unsigned int num) {
    char buf[16]; int i=0;
    if (!num) { print_string("0"); return; }
    do { int d=num%16; buf[i++]=(d<10)?('0'+d):('A'+d-10); num/=16; } while(num);
    buf[i]=0;
    for (int j=0; j<i/2; j++) { char t=buf[j]; buf[j]=buf[i-1-j]; buf[i-1-j]=t; }
    print_string(buf);
}

void fake_ps() {
    print_string("PID  USER   TIME   COMMAND\n");
    print_string("  1  root   0:00   init [TuxOS]\n");
    print_string("  2  root   0:00   kshell\n");
    print_string("  3  root   0:01   idle\n");
}

void fake_kill(const char *args) {
    int pid = 0;
    for (int i=0; args[i]; i++) {
        if(args[i] >= '0' && args[i] <= '9') pid = pid*10 + (args[i]-'0');
    }
    if (pid==0) { print_string("Usage: kill <pid>\n"); return; }
    print_string("kill: process "); char buf[8]; int_to_str(pid,buf); print_string(buf);
    print_string(" terminated successfully.\n");
}

void fake_dmesg() {
    print_string("[ 0.00] TuxOS 0.2.3 booting...\n");
    print_string("[ 0.02] VBE BGA controller override bounded safely.\n");
    print_string("[ 0.10] PS/2 keyboard text metrics updated.\n");
    print_string("[ 0.15] Shell alignment operational.\n");
}

void fake_who() { print_string("root     tty1   Jun 18 11:08\n"); }

void print_rickroll() {
    print_string("Never gonna give you up\nNever gonna let you down\nNever gonna run around and desert you\n");
}

void print_fortune() {
    const char *fortunes[] = {
        "Bug is just a feature waiting to be found.",
        "Tux is silently judging your code.",
        "Segmentation fault (core dumped) ... just kidding.",
        "Don't panic!"
    };
    int n = sizeof(fortunes)/sizeof(fortunes[0]);
    print_string(fortunes[rand()%n]); print_string("\n");
}

void print_ascii_table() {
    for (int i=32; i<127; i+=8) {
        for (int j=0; j<8 && i+j<127; j++) { char buf[8]={i+j,' ',0}; print_string(buf); }
        print_string("\n");
    }
}

int calc_expression(const char *expr) {
    int num1=0, num2=0, i=0; char op=0;
    while (expr[i]==' ') i++;
    while (expr[i]>='0' && expr[i]<='9') { num1 = num1*10 + (expr[i]-'0'); i++; }
    if (expr[i]=='+'||expr[i]=='-'||expr[i]=='*'||expr[i]=='/') { op=expr[i]; i++; }
    else return 0;
    while (expr[i]>='0' && expr[i]<='9') { num2 = num2*10 + (expr[i]-'0'); i++; }
    switch(op) {
        case '+': return num1+num2;
        case '-': return num1-num2;
        case '*': return num1*num2;
        case '/': if (num2) return num1/num2; else return 0;
    }
    return 0;
}

void guess_game() {
    int number = rand_range(1, 100);
    int guess, attempts = 0; char input[32];
    clear_screen();
    print_string("Guess a number between 1 and 100.\n");
    while (1) {
        print_string("Guess: ");
        int len = read_line(input, sizeof(input));
        if (len==0) continue;
        guess = 0;
        for (int i=0; input[i]; i++) guess = guess*10 + (input[i]-'0');
        attempts++;
        if (guess < number) print_string("Too low!\n");
        else if (guess > number) print_string("Too high!\n");
        else {
            print_string("Correct! Attempts: ");
            char buf[8]; int_to_str(attempts, buf);
            print_string(buf); print_string("\n");
            break;
        }
    }
    print_string("Press any key to drop to system...");
    while (!kbhit());
    flush_keyboard();
    clear_screen();
}

void shell() {
    char input[128];
    while (1) {
        print_string("TuxOS> ");
        int len = read_line(input, sizeof(input));
        if (len==0) continue;
        
        char *cmd = input, *args = "";
        for (int i=0; i<len; i++) {
            if (input[i]==' ') { input[i]=0; args=input+i+1; break; }
        }

        if (!strcmp(cmd,"help")) {
            print_string("Commands: help, clear, uname, panic, game\n");
            print_string("System: ps, kill, dmesg, who, free, uptime\n");
            print_string("Utils: calc, hex, random, ascii, strrev, strlen, fortune\n");
            print_string("Fun: rickroll\n");
        }
        else if (!strcmp(cmd,"clear")) clear_screen();
        else if (!strcmp(cmd,"uname")) print_string("TuxOS 0.2.3 (BGA Native Monolithic)\n");
        else if (!strcmp(cmd,"panic")) kernel_panic(strlen(args) > 0 ? args : "Manual terminal trip execution.");
        else if (!strcmp(cmd,"game")) guess_game();
        else if (!strcmp(cmd,"ps")) fake_ps();
        else if (!strcmp(cmd,"kill")) fake_kill(args);
        else if (!strcmp(cmd,"dmesg")) fake_dmesg();
        else if (!strcmp(cmd,"who")) fake_who();
        else if (!strcmp(cmd,"free")) print_string("Total: 64 MB  Free: 52 MB  Used: 12 MB\n");
        else if (!strcmp(cmd,"uptime")) {
            unsigned int diff = get_rtc_epoch() - boot_epoch;
            char buf[16]; int_to_str(diff, buf);
            print_string("Uptime: "); print_string(buf); print_string(" seconds.\n");
        }
        else if (!strcmp(cmd,"calc")) { 
            int res=calc_expression(args); char buf[16]; int_to_str(res,buf); 
            print_string(buf); print_string("\n"); 
        }
        else if (!strcmp(cmd,"hex")) {
            int num=0; const char *p=args; while (*p>='0'&&*p<='9') { num=num*10+(*p-'0'); p++; }
            print_string("0x"); print_hex(num); print_string("\n");
        }
        else if (!strcmp(cmd,"random")) { print_string("0x"); print_hex(rand()); print_string("\n"); }
        else if (!strcmp(cmd,"ascii")) print_ascii_table();
        else if (!strcmp(cmd,"strrev")) { 
            char rev[128]; int l=strlen(args); for (int i=0;i<l;i++) rev[i]=args[l-1-i]; 
            rev[l]=0; print_string(rev); print_string("\n"); 
        }
        else if (!strcmp(cmd,"strlen")) { 
            char buf[8]; int_to_str(strlen(args),buf); print_string(buf); print_string("\n"); 
        }
        else if (!strcmp(cmd,"fortune")) print_fortune();
        else if (!strcmp(cmd,"rickroll")) print_rickroll();
        else { print_string("Unknown command. Type 'help'.\n"); }
    }
}

void kernel_main(uint32_t* vram) {
    set_video_mode(800, 600, 32);

    if ((uint32_t)vram < 0x00100000) {
        framebuffer = (uint8_t*)0xFD000000; 
    } else {
        framebuffer = (uint8_t*)vram;
    }
    
    clear_screen();
    
    unsigned int seed;
    asm volatile ("rdtsc" : "=A"(seed));
    srand(seed);
    boot_epoch = get_rtc_epoch();   
    
    print_string("Welcome to TuxOS 0.2.3!\n\n");
    
    shell();
    while(1){}
}
