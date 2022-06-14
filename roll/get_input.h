#ifndef GET_INPUT_H
#define GET_INPUT_H
// This is spiritually a single-header library, 
// but it needs <Windows.h> on windows, so to help isolate from
// that, it is implemented as .h and .c
//
// If you need them as externs, just trivially wrap them.

// size_t
#include <stddef.h>
#include "long_string.h"

#ifdef _WIN32
// allow user to suppress this def
#ifndef HAVE_SSIZE_T
typedef long long ssize_t;
#endif
#else
// ssize_t
#include <sys/types.h>
#endif

#ifdef __clang__
#pragma clang assume_nonnull begin
#endif


enum {LINE_HISTORY_MAX = 100};
struct LineHistory {
    int count;
    int cursor;
    LongString history[LINE_HISTORY_MAX];
};
// Returns non-zero if there was an error.
static int dump_history(struct LineHistory*);
// Returns non-zero if there was an error.
static int load_history(struct LineHistory*);
static void add_line_to_history(struct LineHistory*, LongString line);
static ssize_t get_input_line(struct LineHistory*, LongString prompt, char* buff, size_t buff_len);
static int get_cols(void);

#ifdef __clang__
#pragma clang assume_nonnull end
#endif

#endif
