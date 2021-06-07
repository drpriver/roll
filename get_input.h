#ifndef GET_INPUT_H
#define GET_INPUT_H
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <conio.h>
#include <io.h>
typedef long long ssize_t;
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#else

#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#endif

#include "david_macros.h"
#include "long_string.h"


static LongString input_line_history[1000];
static size_t input_line_history_count = 0;
static size_t input_line_history_cursor = 0;
static int get_line_is_init;
static void get_line_init(void);

static size_t get_line_internal(char* buff, size_t buffsize, LongString prompt);
static size_t get_line_internal_loop(char* buff, size_t buffsize, LongString prompt);
struct TermState;
static void enable_raw(struct TermState*);
static void disable_raw(struct TermState*);

static inline
ssize_t
read_one(char* buff){
#ifdef _WIN32
    *buff = _getch();
    return 1;
#else
    return read(STDIN_FILENO, buff, 1);
#endif
    }

static inline
ssize_t
write_data(char*buff, size_t len){
#ifdef _WIN32
    for(size_t i = 0; i < len; i++){
        _putch(buff[i]);
        }
#else
    return write(STDOUT_FILENO, buff, len);
#endif
    }

static inline
void*
memdup(const void* src, size_t size){
    if(!size) return NULL;
    void* p = malloc(size);
    if(p) memcpy(p, src, size);
    return p;
}

static
LongString
get_input_line(LongString prompt, int display_size){
    char buff[4096];
    size_t length = get_line_internal(buff, sizeof(buff), prompt);
    if(!length){
        return (LongString){0, 0};
    }
    return (LongString){.length=length, .text=memdup(buff, length+1)};
}

static size_t get_line_internal(char* buff, size_t buffsize, LongString prompt){
    if(!get_line_is_init)
        get_line_init();
    struct TermState termstate;
    enable_raw(&termstate);
    size_t result_length = get_line_internal_loop(buff, buffsize, prompt);
    disable_raw(&termstate);
    return result_length;
    }

#ifdef _WIN32
struct TermState {
    };
static void enable_raw(struct TermState*ts){
    (void)ts;
    }
static void disable_raw(struct TermState*ts){
    (void)ts;
    }
#else
struct TermState {
    struct termios raw;
    struct termios orig;
};
struct LineState {
    char* buff;
    size_t buffsize;
    LongString prompt;
    size_t curr_pos;
    size_t old_pos;
    size_t length;
    size_t cols;
    int history_index;
};
static void enable_raw(struct TermState*ts){
    if(tcgetattr(STDIN_FILENO, &ts->orig) == -1)
        return;
    ts->raw = ts->orig;
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    ts->raw.c_iflag &= ~(BRKINT|ICRNL|INPCK|ISTRIP|IXON);
    /* output modes - disable post processing */
    ts->raw.c_oflag &= ~OPOST;
    /* control modes - set 8 bit chars */
    ts->raw.c_cflag |= CS8;
    /* local modes - echoing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    ts->raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer.
     * We want read to return every single byte, without timeout. */
    ts->raw.c_cc[VMIN] = 1;
    ts->raw.c_cc[VTIME] = 0;

    /* put terminal in raw mode after flushing */
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ts->raw) < 0)
        return;
    }
static void disable_raw(struct TermState*ts){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &ts->orig);
}


#endif

static void redisplay(struct LineState*);
static
size_t
get_line_internal_loop(char* buff, size_t buffsize, LongString prompt){
    struct LineState ls = {
        .buff = buff,
        .buffsize = buffsize-1, // -1 for terminating nul
        .prompt = prompt,
        .cols = get_cols(),
    };
    buff[0] = '\0';
    write_data(prompt.text, prompt.length);
    enum {
        CTRL_A = 1,         // Ctrl+a
        CTRL_B = 2,         // Ctrl-b
        CTRL_C = 3,         // Ctrl-c
        CTRL_D = 4,         // Ctrl-d
        CTRL_E = 5,         // Ctrl-e
        CTRL_F = 6,         // Ctrl-f
        CTRL_H = 8,         // Ctrl-h
        TAB = 9,            // Tab
        CTRL_K = 11,        // Ctrl+k
        CTRL_L = 12,        // Ctrl+l
        ENTER = 13,         // Enter
        CTRL_N = 14,        // Ctrl-n
        CTRL_P = 16,        // Ctrl-p
        CTRL_T = 20,        // Ctrl-t
        CTRL_U = 21,        // Ctrl-u
        CTRL_W = 23,        // Ctrl-w
        ESC = 27,           // Escape
        BACKSPACE =  127    // Backspace
    };
    for(;;){
        char c;
        ssize_t nread = read_one(&c);
        if(nread <= 0)
            return ls.length;
        // ignore tabs.
        if(c == TAB)
            continue;
        switch(c){
            case ENTER:
                //TODO:
                break;
            case BACKSPACE: case CTRL_H:
                if(ls.curr_pos > 0 && ls.length > 0){
                    memmove(ls.buff+ls.curr_pos-1, ls.buff+ls.curr_pos, ls.length-ls.curr_pos);
                    ls.curr_pos--;
                    ls.buff[--ls.length] = '\0';
                    redisplay(&ls);
                }
                break;
            }
    }
}

static 
void 
redisplay(struct LineState*ls){
}


#endif
