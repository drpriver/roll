#ifndef GET_INPUT_H
#define GET_INPUT_H
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

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


static int get_line_is_init;
static void get_line_init(void);
static int get_cols(void);

static ssize_t get_line_internal(char* buff, size_t buffsize, LongString prompt);
static ssize_t get_line_internal_loop(char* buff, size_t buffsize, LongString prompt);
struct TermState;
static void enable_raw(struct TermState*);
static void disable_raw(struct TermState*);

enum {LINE_HISTORY_MAX = 100};
static LongString input_line_history[LINE_HISTORY_MAX];
static int input_line_history_count = 0;
static int input_line_history_cursor = 0;
enum HistoryDirection {
    PREVIOUS_LINE,
    NEXT_LINE,
};
struct LineState;
static void change_history(struct LineState*, int magnitude);

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

#if 0
FILE* loop_fp;
void vdbg(const char* fmt, va_list args){
    if(!loop_fp){
        loop_fp = fopen("debug.txt", "a");
        setbuf(loop_fp, NULL);
        }
    vfprintf(loop_fp, fmt, args);
    }
__attribute__((format(printf,1, 2)))
void dbg(const char*fmt, ...){
    va_list args;
    va_start(args, fmt);
    vdbg(fmt, args);
    va_end(args);
    }
#define DBG(fmt, ...) dbg("%s:%3d | " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define DBG(...) (void)0
#endif

static
LongString
get_input_line(LongString prompt){
    char buff[4096];
    input_line_history_cursor = input_line_history_count;
    ssize_t length = get_line_internal(buff, sizeof(buff), prompt);
    if(length < 0){
        return (LongString){0, 0};
    }
    return (LongString){.length=length, .text=memdup(buff, length+1)};
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
    size_t length;
    size_t cols;
    int history_index;
};
static 
void 
enable_raw(struct TermState*ts){
    if(tcgetattr(STDIN_FILENO, &ts->orig) == -1)
        return;
    ts->raw = ts->orig;
    ts->raw.c_iflag &= ~(0
            | BRKINT // no break
            | ICRNL  // don't map CR to NL
            | INPCK  // skip parity check
            | ISTRIP // don't strip 8th bit off char
            | IXON   // don't allow start/stop input control
            );
    ts->raw.c_oflag &= ~(0
        | OPOST // disable post processing
        );
    ts->raw.c_cflag |= CS8; // 8 bit chars
    ts->raw.c_lflag &= ~(0
            | ECHO    // disable echo
            | ICANON  // disable canonical processing
            | IEXTEN  // no extended functions
            // Currently allowing these so ^Z works, could disable them
            // | ISIG    // disable signals
            );
    ts->raw.c_cc[VMIN] = 1; // read every single byte
    ts->raw.c_cc[VTIME] = 0; // no timeout

    // set and flush
    // Change will ocurr after all output has been transmitted.
    // Unread input will be discarded.
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ts->raw) < 0)
        return;
    }
static void disable_raw(struct TermState*ts){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &ts->orig);
}
#endif

static ssize_t get_line_internal(char* buff, size_t buffsize, LongString prompt){
    if(!get_line_is_init)
        get_line_init();
    struct TermState termstate;
    enable_raw(&termstate);
    ssize_t result_length = get_line_internal_loop(buff, buffsize, prompt);
    disable_raw(&termstate);
    return result_length;
    }


static void redisplay(struct LineState*);
static void delete_right(struct LineState*);
static void insert_char_into_line(struct LineState*, char);
static
ssize_t
get_line_internal_loop(char* buff, size_t buffsize, LongString prompt){
    DBG("Enter Loop\n");
    DBG("----------\n");
    struct LineState ls = {
        .buff = buff,
        .buffsize = buffsize-1, // -1 for terminating nul
        .prompt = prompt,
        .cols = get_cols(),
    };
    write_data(prompt.text, prompt.length);
    memset(buff, 0, buffsize);
    buff[0] = '\0';
    redisplay(&ls);
    enum {
        CTRL_A = 1,         // Ctrl-a
        CTRL_B = 2,         // Ctrl-b
        CTRL_C = 3,         // Ctrl-c
        CTRL_D = 4,         // Ctrl-d
        CTRL_E = 5,         // Ctrl-e
        CTRL_F = 6,         // Ctrl-f
        CTRL_H = 8,         // Ctrl-h
        TAB = 9,            // Tab
        CTRL_K = 11,        // Ctrl-k
        CTRL_L = 12,        // Ctrl-l
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
        DBG("buff: '%.*s'\n", (int)ls.length, ls.buff);
        DBG("curr_pos: %zu\n", ls.curr_pos);
        DBG("length: %zu\n", ls.length);
        char c;
        char sequence[8];
        ssize_t nread = read_one(&c);
        if(nread <= 0)
            return ls.length;
        DBG("c = '%c' (%d)\n", isprint(c)?c:' ', c);
        // ignore tabs.
        if(c == TAB)
            continue;
        switch(c){
            case ENTER:
                DBG("ENTER\n");
                write_data("\n", 1);
                return ls.length;
            case BACKSPACE: case CTRL_H:
                DBG("BACKSPACE\n");
                if(ls.curr_pos > 0 && ls.length > 0){
                    DBG("ls.curr_pos = %zu\n", ls.curr_pos);
                    DBG("ls.length = %zu\n", ls.length);
                    memmove(ls.buff+ls.curr_pos-1, ls.buff+ls.curr_pos, ls.length-ls.curr_pos);
                    ls.curr_pos--;
                    ls.buff[--ls.length] = '\0';
                    DBG("'%.*s'\n", (int)ls.length, ls.buff);
                    DBG("'%.*s'\n", (int)strlen(ls.buff), ls.buff);
                    redisplay(&ls);
                }
                break;
            case CTRL_D:
                DBG("CTRL_D\n");
                if(ls.length > 0){
                    // TODO: delete right
                    delete_right(&ls);
                    redisplay(&ls);
                    }
                else {
                    return -1;
                    }
                break;
            case CTRL_T:
                DBG("CTRL_T\n");
                if(ls.curr_pos > 0 && ls.curr_pos < ls.length){
                    // swap with previous
                    char temp = buff[ls.curr_pos-1];
                    buff[ls.curr_pos-1] = buff[ls.curr_pos];
                    buff[ls.curr_pos] = temp;
                    if (ls.curr_pos != ls.length-1) 
                        ls.curr_pos++;
                    redisplay(&ls);
                }
                break;
            case CTRL_B:
                DBG("CTRL_B\n");
                if(ls.curr_pos > 0){
                    ls.curr_pos--;
                    redisplay(&ls);
                }
                break;
            case CTRL_F:
                DBG("CTRL_F\n");
                if(ls.curr_pos != ls.length){
                    ls.curr_pos++;
                    redisplay(&ls);
                }
                break;
            case CTRL_P:
                DBG("CTRL_P\n");
                change_history(&ls, -1);
                redisplay(&ls);
                break;
            case CTRL_N:
                DBG("CTRL_N\n");
                change_history(&ls, +1);
                redisplay(&ls);
                break;
            case ESC: // beginning of escape sequence
                DBG("ESC\n");
                if(read_one(sequence) == -1) return -1;
                if(read_one(sequence+1) == -1) return -1;

                // ESC [ sequences
                if(sequence[0] == '['){
                    if (sequence[1] >= '0' && sequence[1] <= '9') {
                        // Extended escape, read additional byte.
                        if (read_one(sequence+2) == -1) return -1;
                        if (sequence[2] == '~') {
                            switch(sequence[1]) {
                            case '3': /* Delete key. */
                                delete_right(&ls);
                                redisplay(&ls);
                                break;
                            }
                        }
                    } 
                    else {
                        switch(sequence[1]) {
                        case 'A': // Up
                            change_history(&ls, -1);
                            redisplay(&ls);
                            break;
                        case 'B': // Down
                            change_history(&ls, +1);
                            redisplay(&ls);
                            break;
                        case 'C': // Right
                            if(ls.curr_pos != ls.length){
                                ls.curr_pos++;
                                redisplay(&ls);
                            }
                            break;
                        case 'D': // Left
                            if(ls.curr_pos > 0){
                                ls.curr_pos--;
                                redisplay(&ls);
                            }
                            break;
                        case 'H': // Home
                            ls.curr_pos = 0;
                            redisplay(&ls);
                            break;
                        case 'F': // End
                            ls.curr_pos = ls.length;
                            redisplay(&ls);
                            break;
                        }
                    }
                }
                else if(sequence[0] == 'O'){
                    switch(sequence[1]){
                        case 'H': // Home
                            ls.curr_pos = 0;
                            redisplay(&ls);
                            break;
                        case 'F': // End
                            ls.curr_pos = ls.length;
                            redisplay(&ls);
                            break;
                        }
                    }
                break;
            default:
                DBG("default ('%c')\n", c);
                insert_char_into_line(&ls, c);
                redisplay(&ls);
                break;
            case CTRL_C:
                DBG("CTRL_C\n");
                buff[0] = '\0';
                ls.curr_pos = 0;
                ls.length = 0;
                redisplay(&ls);
                break;
            case CTRL_U: // Delete entire line
                DBG("CTRL_U\n");
                buff[0] = '\0';
                ls.curr_pos = 0;
                ls.length = 0;
                redisplay(&ls);
                break;
            case CTRL_K: // Delete to end of line
                DBG("CTRL_K\n");
                buff[ls.curr_pos] = '\0';
                ls.length = ls.curr_pos;
                redisplay(&ls);
                break;
            case CTRL_A: // Home
                DBG("CTRL_A\n");
                ls.curr_pos = 0;
                redisplay(&ls);
                break;
            case CTRL_E: // End
                DBG("CTRL_E\n");
                ls.curr_pos = ls.length;
                redisplay(&ls);
                break;
            case CTRL_L: // Clear entire screen
                DBG("CTRL_L\n");
                #define CLEARSCREEN "\x1b[H\x1b[2J"
                write_data(CLEARSCREEN, sizeof(CLEARSCREEN)-1);
                #undef CLEARSCREEN
                redisplay(&ls);
                break;
            case CTRL_W:{ // Delete previous word
                DBG("CTRL_W\n");
                size_t old_pos = ls.curr_pos;
                size_t diff;
                size_t pos = ls.curr_pos;
                // Backup until we hit a nonspace.
                while(pos > 0 && buff[pos-1] == ' ')
                    pos--;
                // Backup until we hit a space.
                while(pos > 0 && buff[pos-1] != ' ')
                    pos--;
                diff = old_pos - pos;
                memmove(buff+pos, buff+old_pos, ls.length-old_pos+1);
                ls.curr_pos = pos;
                ls.length -= diff;
                redisplay(&ls);
                }break;
            }
    }
    return ls.length;
}

static 
void 
redisplay(struct LineState*ls){
    enum {LINESIZE=1024};
    char seq[LINESIZE];
    int seq_pos = 0;
    size_t plen = ls->prompt.length;
    char* buff = ls->buff;
    size_t len = ls->length;
    size_t pos = ls->curr_pos;
    size_t cols = ls->cols;

    // Scroll the text right until the current cursor position
    // fits on screen.
    while((plen+pos) >= cols) {
        buff++;
        len--;
        pos--;
    }
    // Truncate the string so it fits on screen.
    while (plen+len > cols) {
        len--;
    }
    // Move to left.
    seq[seq_pos++] = '\r';

    // Copy the prompt.
    if(plen+seq_pos < LINESIZE){
        DBG("seq_pos: %d\n", seq_pos);
        DBG("prompt: '%.*s'\n", (int)plen, ls->prompt.text);
        memcpy(seq+seq_pos, ls->prompt.text, plen);
        seq_pos += plen;
        }
    else {
        DBG("plen+seq_pos >= LINESIZE\n");
        return;
        }

    // Copy the visible section of the buffer.
    if(seq_pos + len < LINESIZE){
        DBG("seq_pos: %d\n", seq_pos);
        DBG("buff: '%.*s'\n", (int)len, buff);
        memcpy(seq+seq_pos, buff, len);
        seq_pos += len;
        }
    else {
        DBG("seq_pos + len >= LINESIZE\n");
        return;
        }
    // Erase anything remaining on this line to the right.
    #define ERASERIGHT "\x1b[0K"
    if(seq_pos + sizeof(ERASERIGHT)-1 < LINESIZE){
        DBG("seq_pos: %d\n", seq_pos);
        memcpy(seq+seq_pos, ERASERIGHT, sizeof(ERASERIGHT)-1);
        seq_pos += sizeof(ERASERIGHT)-1;
        }
    else {
        DBG("seq_pos + sizeof(ERASERIGHT)-1 >= LINESIZE\n");
        return;
        }
    #undef ERASERIGHT
    // Move cursor back to original position.
    DBG("seq_pos: %d\n", seq_pos);
    DBG("pos+plen: %zu\n", pos+plen);
    int printsize = snprintf(seq+seq_pos, LINESIZE-seq_pos, "\r\x1b[%zuC", pos+plen);
    DBG("printsize: %d\n", printsize);
    if(printsize > LINESIZE-seq_pos){
        DBG("printsize > LINESIZE-seq_pos\n");
        return;
        }
    else
        seq_pos += printsize;
    // Actually write to the terminal.
    DBG("seq_pos: %d\n", seq_pos);
    write_data(seq, seq_pos);
}

static 
void 
delete_right(struct LineState* ls){
    if(ls->length > 0 && ls->curr_pos < ls->length) {
        char* buff = ls->buff;
        size_t pos = ls->curr_pos;
        memmove(buff+pos, buff+pos+1,ls->length-pos-1);
        buff[--ls->length] = '\0';
    }
}

static 
void 
insert_char_into_line(struct LineState* ls, char c){
    if(ls->length >= ls->buffsize)
        return;
    // At the end of the line anyway
    if(ls->length == ls->curr_pos){
        ls->buff[ls->curr_pos++] = c;
        ls->buff[++ls->length] = '\0';
        return;
        }
    // Write into the middle of the buffer
    memmove(ls->buff+ls->curr_pos+1,ls->buff+ls->curr_pos,ls->length-ls->curr_pos);
    ls->buff[ls->curr_pos++] = c;
    ls->buff[++ls->length] = '\0';
    }

static 
void 
line_history_push(LongString ls){
    if(!ls.length)
        return; // no empties
    LongString* last = &input_line_history[input_line_history_count];
    if(ls.length == last->length && memcmp(ls.text, last->text, ls.length) == 0)
        return; // Don't allow duplicates
    char* copy = memdup(ls.text, ls.length+1);
    if(input_line_history_count == LINE_HISTORY_MAX){
        free(input_line_history[0].text);
        memmove(input_line_history, input_line_history+1, (LINE_HISTORY_MAX-1)*sizeof(input_line_history[0]));
        input_line_history[LINE_HISTORY_MAX-1] = (LongString){
            .length = ls.length,
            .text = copy,
            };
        }
    else {
        input_line_history[input_line_history_count++] = (LongString){.length=ls.length, .text=copy};
        }
    }

static inline
void
add_line_to_history(LongString line){
    line_history_push(line);
    }


static 
void 
change_history(struct LineState* ls, int magnitude){
    DBG("magnitude: %d\n", magnitude);
    DBG("input_line_history_cursor: %d\n", input_line_history_cursor);
    DBG("input_line_history_count: %d\n", input_line_history_count);
    input_line_history_cursor += magnitude;
    if(input_line_history_cursor < 0)
        input_line_history_cursor = 0;
    if(input_line_history_cursor >= input_line_history_count){
        input_line_history_cursor = input_line_history_count;
        ls->length = 0;
        ls->curr_pos = 0;
        ls->buff[ls->length] = '\0';
        return;
        }
    if(input_line_history_cursor < 0)
        return;
    LongString old = input_line_history[input_line_history_cursor];
    size_t length = old.length < ls->buffsize? old.length : ls->buffsize;
    if(length)
        memcpy(ls->buff, old.text, length);
    ls->buff[length] = '\0';
    ls->length = length;
    ls->curr_pos = length;
    }
static void get_line_init(void){
    get_line_is_init = true;
    }
static int 
get_cols(void){
    return 80;
    }


#endif
