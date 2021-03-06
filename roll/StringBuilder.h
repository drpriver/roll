//
// Copyright © 2021-2022, David Priver
//
#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include "common_macros.h"
#include "long_string.h"

#ifdef __clang__
#pragma clang assume_nonnull begin
#else
#ifndef _Null_unspecified
#define _Null_unspecified
#endif
#endif

typedef struct StringBuilder {
    size_t cursor;
    size_t capacity;
    char*_Null_unspecified data;
} StringBuilder;

static inline
void
sb_destroy(StringBuilder* sb){
    free(sb->data);
    sb->data=0;
    sb->cursor=0;
    sb->capacity=0;
}

static inline
void
_check_sb_size(StringBuilder*, size_t);

static inline
void
sb_nul_terminate(StringBuilder* sb){
    _check_sb_size(sb, 1);
    sb->data[sb->cursor] = '\0';
}

static inline
LongString
sb_borrow(Nonnull(StringBuilder*) sb){
    sb_nul_terminate(sb);
    assert(sb->data);
    return (LongString) {
        .text = sb->data,
        .length = sb->cursor,
    };
}

static inline
void
sb_reset(StringBuilder* sb){
    sb->cursor = 0;
}

static inline
void
_resize_sb(StringBuilder* sb, size_t size){
    char* new_data = realloc(sb->data, size);
    assert(new_data);
    sb->data = new_data;
    sb->capacity = size;
}

static inline
void
_check_sb_size(StringBuilder* sb, size_t len){
    if (sb->cursor + len > sb->capacity){
        size_t new_size = (sb->capacity*3)/2;
        if(new_size < 32)
            new_size = 32;
        while(new_size < sb->cursor+len){
            new_size *= 2;
        }
        _resize_sb(sb, new_size);
    }
}

static inline
void
sb_write_str(StringBuilder* restrict sb, const char* restrict str, size_t len){
    if(! len)
        return;
    _check_sb_size(sb, len);
    memcpy(sb->data + sb->cursor, str, len);
    sb->cursor += len;
}

static inline
void
sb_rstrip(StringBuilder* sb){
    while(sb->cursor){
        char c = sb->data[sb->cursor-1];
        switch(c){
            case ' ': case '\n': case '\t': case '\r':
                sb->cursor--;
                sb->data[sb->cursor] = '\0';
                break;
            default:
                return;
        }
    }
}

static inline
void
sb_lstrip(StringBuilder* sb){
    int n_whitespace = 0;
    for(size_t i = 0; i < sb->cursor; i++){
        switch(sb->data[i]){
            case ' ': case '\n': case '\t': case '\r':
                n_whitespace++;
                break;
            default:
                goto endloop;
        }
    }
    endloop:;
    if(!n_whitespace)
        return;
    memmove(sb->data, sb->data+n_whitespace, sb->cursor-n_whitespace);
    sb->cursor-= n_whitespace;
    return;
}

static inline
void
sb_strip(StringBuilder* sb){
    sb_rstrip(sb);
    sb_lstrip(sb);
}
#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
