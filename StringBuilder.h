#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include "david_macros.h"
#include "long_string.h"
typedef struct StringBuilder {
    size_t cursor;
    size_t capacity;
    NullUnspec(char*) data;
    } StringBuilder;

static inline
void
sb_destroy(Nonnull(StringBuilder*) sb){
    free(sb->data);
    sb->data=0;
    sb->cursor=0;
    sb->capacity=0;
    }

static inline
void
_check_sb_size(Nonnull(StringBuilder*), size_t);

static inline
void
sb_nul_terminate(Nonnull(StringBuilder*) sb){
    _check_sb_size(sb, 1);
    sb->data[sb->cursor] = '\0';
    }


static inline
LongString
sb_detach(Nonnull(StringBuilder*) sb){
    assert(sb->data);
    sb_nul_terminate(sb);
    LongString result = {};
    result.text = sb->data;
    result.length = sb->cursor;
    sb->data = NULL;
    sb->capacity = 0;
    sb->cursor = 0;
    return result;
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
size_t
sb_len(Nonnull(StringBuilder*)sb){
    return sb->cursor;
    }

static inline
void
sb_reset(Nonnull(StringBuilder*) sb){
    sb->cursor = 0;
    }

static inline
StringBuilder
create_sb(size_t size){
    StringBuilder sb = {
        .data = malloc(size),
        .capacity=size,
        .cursor = 0,
        };
    assert(sb.data);
    return sb;
    }

static inline
void
_resize_sb(Nonnull(StringBuilder*) sb, size_t size){
    char* new_data = realloc(sb->data, size);
    assert(new_data);
    sb->data = new_data;
    sb->capacity = size;
    }

static inline
void
_check_sb_size(Nonnull(StringBuilder*) sb, size_t len){
    if (sb->cursor + len > sb->capacity){
        size_t new_size = Max_literal((sb->capacity*3)/2, 32);
        while(new_size < sb->cursor+len){
            new_size *= 2;
            }
        _resize_sb(sb, new_size);
        }
    }

static inline
void
sb_write_str(Nonnull(StringBuilder*) restrict sb, Nonnull(const char*) restrict str, size_t len){
    if(not len)
        return;
    _check_sb_size(sb, len);
    memcpy(sb->data + sb->cursor, str, len);
    sb->cursor += len;
    }

#define sb_write_literal(sb, lit) sb_write_str(sb, "" lit, sizeof(lit)-1)

static inline
void
sb_write_bytes(Nonnull(StringBuilder*) restrict sb, Nonnull(const void*) restrict v, size_t len){
    _check_sb_size(sb, len);
    memcpy(sb->data+sb->cursor, v, len);
    sb->cursor += len;
    }

static inline
void
sb_write_cstr(Nonnull(StringBuilder*) restrict sb, Nonnull(const char*) restrict str){
    auto len = strlen(str);
    sb_write_str(sb, str, len);
    }

static inline
void
sb_write_char(Nonnull(StringBuilder*) sb, char c){
    _check_sb_size(sb, 1);
    sb->data[sb->cursor++] = c;
    }
static inline
void
sb_insert_char(Nonnull(StringBuilder*) sb, int index, char c){
    _check_sb_size(sb, 1);
    int move_length = sb->cursor - index;
    if(move_length)
        memmove(sb->data+index+1, sb->data+index, move_length);
    sb->data[index] = c;
    sb->cursor++;
    }
static inline
void
sb_erase_char_at(Nonnull(StringBuilder*) sb, int index){
    assert(index >= 0);
    assert(index < sb->cursor);
    int move_length = sb->cursor - index - 1;
    if(move_length){
        memmove(sb->data+index, sb->data+index+1, move_length);
        }
    sb->cursor--;
    }
static inline
void
sb_write_multibyte_char(Nonnull(StringBuilder*) sb, unsigned int c){
    if(c & 0xff000000)
        sb_write_char(sb, (c & 0xff000000)>>24);
    if(c & 0xff0000)
        sb_write_char(sb, (c & 0xff0000)>>16);
    if(c & 0xff00)
        sb_write_char(sb, (c & 0xff00)>>8);
    if(c & 0xff)
        sb_write_char(sb, c & 0xff);
    }

static inline
void
sb_write_repeated_char(Nonnull(StringBuilder*) sb, char c, int n){
    _check_sb_size(sb, n);
    for(int i = 0; i < n; i++){
        sb->data[sb->cursor++] = c;
        }
    }

static inline
char
sb_peek_end(Nonnull(StringBuilder*) sb){
    assert(sb->data);
    return sb->data[sb->cursor-1];
    }

static inline
void
sb_erase(Nonnull(StringBuilder*) sb, size_t len){
    if(len > sb->cursor){
        sb->cursor = 0;
        sb->data[0] = '\0';
        return;
        }
    sb->cursor -= len;
    sb->data[sb->cursor] = '\0';
    }

static inline
void
sb_rstrip(Nonnull(StringBuilder*)sb){
    while(sb->cursor){
        auto c = sb->data[sb->cursor-1];
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
sb_lstrip(Nonnull(StringBuilder*)sb){
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
sb_strip(Nonnull(StringBuilder*)sb){
    sb_rstrip(sb);
    sb_lstrip(sb);
    }

static inline
void
sb_read_file(Nonnull(StringBuilder*) sb, Nonnull(FILE*) restrict fp){
    // do it 1024 bytes at a time? maybe we can do it faster? idk
    enum {SB_READ_FILE_SIZE=1024};
    for(;;){
        _check_sb_size(sb, SB_READ_FILE_SIZE);
        auto numread = fread(sb->data + sb->cursor, 1, SB_READ_FILE_SIZE, fp);
        sb->cursor += numread;
        if (numread != SB_READ_FILE_SIZE)
            break;
        }
    }

printf_func(2, 3)
static inline
int
sb_sprintf(Nonnull(StringBuilder*)sb, Nonnull(const char*) restrict fmt, ...){
    va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);
    auto _msg_size = vsnprintf(NULL, 0, fmt, args)+1;
    va_end(args);
    _check_sb_size(sb, _msg_size);
    auto result = vsprintf(sb->data + sb->cursor, fmt, args2);
    sb->cursor += result;
    va_end(args2);
    return result;
    }

static inline
int
sb_vsprintf(Nonnull(StringBuilder*)sb, Nonnull(const char*) restrict fmt, va_list args){
    va_list args2;
    va_copy(args2, args);
    auto _msg_size = vsnprintf(NULL, 0, fmt, args)+1;
    va_end(args);
    _check_sb_size(sb, _msg_size);
    auto result = vsprintf(sb->data + sb->cursor, fmt, args2);
    sb->cursor += result;
    va_end(args2);
    return result;
    }

static inline
void
sb_ljust(Nonnull(StringBuilder*)sb, char c, int total_length){
    if(sb->cursor > total_length)
        return;
    auto n = total_length - sb->cursor;
    sb_write_repeated_char(sb, c, n);
    }

static inline
void
sb_rjust(Nonnull(StringBuilder*)sb, char c, int total_length){
    if(sb->cursor > total_length)
        return;
    auto n = total_length - sb->cursor;
    _check_sb_size(sb, n);
    memmove(sb->data+n, sb->data, sb->cursor);
    for(int i = 0; i < n; i++){
        sb->data[i] = c;
        }
    sb->cursor +=n;
    }
static inline
void
sb_merge(
        Nonnull(StringBuilder*)restrict sb,
        Nonnull(Nonnull(const char* restrict)*)restrict strings,
        int n_strings
    ){
    assert(n_strings >= 0);
    if(not n_strings)
        return;
    size_t* string_lens = alloca(sizeof(*string_lens)*n_strings);
    size_t total_len = 0;
    for(int i = 0; i < n_strings; i++){
        string_lens[i] = strlen(strings[i]);
        total_len += string_lens[i];
        }
    _check_sb_size(sb, total_len);
    size_t cursor = sb->cursor;
    for(int i = 0; i < n_strings; i++){
        size_t string_len = string_lens[i];
        memcpy(sb->data+cursor, strings[i], string_len);
        cursor += string_len;
        }
    sb->cursor = cursor;
    }

static inline
void
sb_join(
        Nonnull(StringBuilder*)restrict sb,
        Nonnull(const char*)restrict joiner,
        Nonnull(Nonnull(const char* restrict)*)restrict strings,
        int n_strings
    ){
    assert(n_strings >= 0);
    if(not n_strings)
        return;
    auto joiner_len = strlen(joiner);
    if(not joiner_len){
        sb_merge(sb, strings, n_strings);
        return;
        }
    size_t* string_lens = alloca(sizeof(*string_lens)*n_strings);
    size_t total_len = joiner_len * (n_strings-1);
    for(int i = 0; i < n_strings; i++){
        string_lens[i] = strlen(strings[i]);
        total_len += string_lens[i];
        }
    _check_sb_size(sb, total_len);
    size_t cursor = sb->cursor;
    for(int i = 0; i < n_strings; i++){
        if(i != 0){
            memcpy(sb->data+cursor, joiner, joiner_len);
            cursor += joiner_len;
            }
        size_t string_len = string_lens[i];
        memcpy(sb->data+cursor, strings[i], string_len);
        cursor += string_len;
        }
    sb->cursor = cursor;
    }

#endif
