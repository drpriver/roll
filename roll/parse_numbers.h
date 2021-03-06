//
// Copyright © 2021-2022, David Priver
//
#ifndef PARSE_NUMBERS_H
#define PARSE_NUMBERS_H
// size_t
#include <stddef.h>
// bool
#include <stdbool.h>
// integer types
#include <stdint.h>
// INT64_MAX, etc.
#include <limits.h>

//
// Functions for parsing strings into integers.
//
// Features:
//    * Uses string + length instead of nul-terminated strings.
//    * Ignores locale.
//    * Doesn't use errno.
//    * Handles binary notation, hex notation, #hex notation, decimal.
//
// Defects:
//    * Currently only builds with gnuc (gcc, clang) due to use of overflow
//      intrinsics.

#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

#ifndef warn_unused

#if defined(__GNUC__) || defined(__clang__)
#define warn_unused __attribute__((warn_unused_result))
#elif defined(_MSC_VER)
#define warn_unused _Check_return
#else
#define warn_unused
#endif

#endif

// NOTE: The first error that is happened to be detected will be reported.
//       For strings with multiple errors, this is arbitrary.
//       For example, parsing '12dddddddddddddddddddddd12' into an int
//       may report OVERFLOWED_VALUE instead of INVALID_CHARACTER if the length
//       check is done first. There is no guarantee over which particular error
//       is reported, just that an error will be reported for invalid input.
enum ParseNumberError {
    // No error.
    PARSENUMBER_NO_ERROR = 0,
    // Input ended when more input was expected.
    // For example, parsing '0x' as an unsigned, more data is expected after the 'x'.
    PARSENUMBER_UNEXPECTED_END = 1,
    // The result does not fit in the data type.
    PARSENUMBER_OVERFLOWED_VALUE = 2,
    // An invalid character was encountered, like the 'a' in '33a2' when
    // parsing an int.
    PARSENUMBER_INVALID_CHARACTER = 3,
};

//
// Result structs for the various parsing functions.
//
// Check the `errored` field first. If it is non-zero, `result` is
// indeterminate.
// If it is zero, then the result field is valid and is the parsed number.
//

struct Uint64Result {
    uint64_t result;
    enum ParseNumberError errored;
};

struct Int64Result {
    int64_t result;
    enum ParseNumberError errored;
};

struct Uint32Result {
    uint32_t result;
    enum ParseNumberError errored;
};

struct Int32Result {
    int32_t result;
    enum ParseNumberError errored;
};

struct IntResult {
    int result;
    enum ParseNumberError errored;
};

//
// Forward declarations of the public API.

//
// All of these functions take pointer + length for strings. They will not read
// beyond the given length and the pointer does not need to be nul-terminated.
// They do not accept trailing or trailing whitespace.
// Decimal formats accept a single leading '+' or '-'.
// These functions can parse the full range of values for the given data type.

//
// Parses a decimal uint64.
static inline
warn_unused
struct Uint64Result
parse_uint64(const char* str, size_t length);

//
// Parses a decimal int64.
static inline
warn_unused
struct Int64Result
parse_int64(const char* str, size_t length);

//
// Parses a decimal uint32.
static inline
warn_unused
struct Uint32Result
parse_uint32(const char*str, size_t length);


//
// Parses a decimal int32.
static inline
warn_unused
struct Int32Result
parse_int32(const char*str, size_t length);

//
// Parses a decimal int.
static inline
warn_unused
struct IntResult
parse_int(const char* str, size_t length);

//
// Parses hex format, but with a leading '#' instead of '0x'.
static inline
warn_unused
struct Uint64Result
parse_pound_hex(const char* str, size_t length);

//
// Parses traditional hex format, such as '0xf00dface'
static inline
warn_unused
struct Uint64Result
parse_hex(const char* str, size_t length);

//
// Parses binary notation, such as '0b1101'.
static inline
warn_unused
struct Uint64Result
parse_binary(const char* str, size_t length);

//
// Parses an unsigned integer, in whatever format is comfortable for a human.
// Accepts 0x hexes, 0b binary, plain decimals, and also # hexes.
static inline
warn_unused
struct Uint64Result
parse_unsigned_human(const char* str, size_t length);

// Implementations after this point.

static inline
warn_unused
struct Uint64Result
parse_uint64(const char* str, size_t length){
    struct Uint64Result result = {};
    if(!length){
        result.errored = PARSENUMBER_UNEXPECTED_END;
        return result;
    }
    if(*str == '+'){
        str++;
        length--;
    }
    if(!length){
        result.errored = PARSENUMBER_UNEXPECTED_END;
        return result;
    }
    // UINT64_MAX is 18,446,744,073,709,551,615 (20 characters)
    if(length > 20){
        result.errored = PARSENUMBER_OVERFLOWED_VALUE;
        return result;
    }
    int bad = false;
    uint64_t value = 0;
    for(size_t i=0;i < length-1; i++){
        unsigned cval = str[i];
        cval -= '0';
        if(cval > 9u)
            bad = true;
        value *= 10;
        value += cval;
    }
    if(bad){
        result.errored = PARSENUMBER_INVALID_CHARACTER;
        return result;
    }
    // Handle the last char differently as it's the only
    // one that can overflow.
    {
        unsigned cval = str[length-1];
        cval -= '0';
        if(cval > 9u){
            result.errored = PARSENUMBER_INVALID_CHARACTER;
            return result;
        }
        if(__builtin_mul_overflow(value, 10, &value)){
            result.errored = PARSENUMBER_OVERFLOWED_VALUE;
            return result;
        }
        if(__builtin_add_overflow(value, cval, &value)){
            result.errored = PARSENUMBER_OVERFLOWED_VALUE;
            return result;
        }

    }
    result.result = value;
    return result;
}

static inline
warn_unused
struct Int64Result
parse_int64(const char* str, size_t length){
    struct Int64Result result = {};
    if(!length){
        result.errored = PARSENUMBER_UNEXPECTED_END;
        return result;
    }
    bool negative = (*str == '-');
    if(negative){
        str++;
        length--;
    }
    else if(*str == '+'){
        str++;
        length--;
    }
    if(!length){
        result.errored = PARSENUMBER_UNEXPECTED_END;
        return result;
    }
    // INT64_MAX is 9223372036854775807 (19 characters)
    if(length > 19){
        result.errored = PARSENUMBER_OVERFLOWED_VALUE;
        return result;
    }
    int bad = false;
    uint64_t value = 0;
    for(size_t i=0;i < length-1; i++){
        unsigned cval = str[i];
        cval -= '0';
        if(cval > 9u)
            bad = true;
        value *= 10;
        value += cval;
    }
    if(bad){
        result.errored = PARSENUMBER_INVALID_CHARACTER;
        return result;
    }
    // Handle the last char differently as it's the only
    // one that can overflow.
    {
        unsigned cval = str[length-1];
        cval -= '0';
        if(cval > 9u){
            result.errored = PARSENUMBER_INVALID_CHARACTER;
            return result;
        }
        if(__builtin_mul_overflow(value, 10, &value)){
            result.errored = PARSENUMBER_OVERFLOWED_VALUE;
            return result;
        }
        if(__builtin_add_overflow(value, cval, &value)){
            result.errored = PARSENUMBER_OVERFLOWED_VALUE;
            return result;
        }
    }
    if(negative){
        if(value > (uint64_t)INT64_MAX+1){
            result.errored = PARSENUMBER_OVERFLOWED_VALUE;
            return result;
        }
        value *= -1;
    }
    else{
        if(value > (uint64_t)INT64_MAX){
            result.errored = PARSENUMBER_OVERFLOWED_VALUE;
            return result;
        }
    }
    result.result = value;
    return result;
}

static inline
warn_unused
struct Uint32Result
parse_uint32(const char*str, size_t length){
    struct Uint32Result result = {};
    if(!length){
        result.errored = PARSENUMBER_UNEXPECTED_END;
        return result;
    }
    if(*str == '+'){
        str++;
        length--;
    }
    if(!length){
        result.errored = PARSENUMBER_UNEXPECTED_END;
        return result;
    }
    // UINT32_MAX is 10 characters
    if(length > 10){
        result.errored = PARSENUMBER_OVERFLOWED_VALUE;
        return result;
    }
    int bad = false;
    uint32_t value = 0;
    for(size_t i=0;i < length-1; i++){
        unsigned cval = str[i];
        cval -= '0';
        if(cval > 9u)
            bad = true;
        value *= 10;
        value += cval;
    }
    if(bad){
        result.errored = PARSENUMBER_INVALID_CHARACTER;
        return result;
    }
    // Handle the last char differently as it's the only
    // one that can overflow.
    {
        unsigned cval = str[length-1];
        cval -= '0';
        if(cval > 9u){
            result.errored = PARSENUMBER_INVALID_CHARACTER;
            return result;
        }
        if(__builtin_mul_overflow(value, 10, &value)){
            result.errored = PARSENUMBER_OVERFLOWED_VALUE;
            return result;
        }
        if(__builtin_add_overflow(value, cval, &value)){
            result.errored = PARSENUMBER_OVERFLOWED_VALUE;
            return result;
        }
    }
    result.result = value;
    return result;
}

static inline
warn_unused
struct Int32Result
parse_int32(const char*str, size_t length){
    struct Int32Result result = {};
    if(!length){
        result.errored = PARSENUMBER_UNEXPECTED_END;
        return result;
    }
    bool negative = (*str == '-');
    if(negative){
        str++;
        length--;
    }
    else if (*str == '+'){
        str++;
        length--;
    }
    if(!length){
        result.errored = PARSENUMBER_UNEXPECTED_END;
        return result;
    }
    // INT32_max is 10 chars
    if(length > 10){
        result.errored = PARSENUMBER_OVERFLOWED_VALUE;
        return result;
    }
    int bad = false;
    uint32_t value = 0;
    for(size_t i=0;i < length-1; i++){
        unsigned cval = str[i];
        cval -= '0';
        if(cval > 9u)
            bad = true;
        value *= 10;
        value += cval;
    }
    if(bad){
        result.errored = PARSENUMBER_INVALID_CHARACTER;
        return result;
    }
    // Handle the last char differently as it's the only
    // one that can overflow.
    {
        unsigned cval = str[length-1];
        cval -= '0';
        if(cval > 9u){
            result.errored = PARSENUMBER_INVALID_CHARACTER;
            return result;
        }
        if(__builtin_mul_overflow(value, 10, &value)){
            result.errored = PARSENUMBER_OVERFLOWED_VALUE;
            return result;
        }
        if(__builtin_add_overflow(value, cval, &value)){
            result.errored = PARSENUMBER_OVERFLOWED_VALUE;
            return result;
        }
    }
    if(negative){
        if(value > (uint32_t)INT32_MAX+1){
            result.errored = PARSENUMBER_OVERFLOWED_VALUE;
            return result;
        }
        value *= -1;
    }
    else{
        if(value > (uint32_t)INT32_MAX){
            result.errored = PARSENUMBER_OVERFLOWED_VALUE;
            return result;
        }
    }
    result.result = value;
    return result;
}

static inline
warn_unused
struct IntResult
parse_int(const char* str, size_t length){
    struct IntResult result;
    struct Int32Result e = parse_int32(str, length);
    result.errored = e.errored;
    result.result = e.result;
    return result;
}

static inline
warn_unused
struct Uint64Result
parse_hex_inner(const char* str, size_t length){
    struct Uint64Result result = {};
    if(length > sizeof(result.result)*2){
        result.errored = PARSENUMBER_OVERFLOWED_VALUE;
        return result;
    }
    uint64_t value = 0;
    for(size_t i = 0; i < length; i++){
        char c = str[i];
        uint64_t char_value;
        switch(c){
            case '0'...'9':
                char_value = c - '0';
                break;
            case 'a'...'f':
                char_value = c - 'a' + 10;
                break;
            case 'A'...'F':
                char_value = c - 'A' + 10;
                break;
            default:
                result.errored = PARSENUMBER_INVALID_CHARACTER;
                return result;
        }
        value <<= 4;
        value |= char_value;
    }
    result.result = value;
    return result;
}

static inline
warn_unused
struct Uint64Result
parse_pound_hex(const char* str, size_t length){
    struct Uint64Result result = {};
    if(length < 2){
        result.errored = PARSENUMBER_UNEXPECTED_END;
        return result;
    }
    if(str[0] != '#'){
        result.errored = PARSENUMBER_INVALID_CHARACTER;
        return result;
    }
    return parse_hex_inner(str+1, length-1);
}

static inline
warn_unused
struct Uint64Result
parse_hex(const char* str, size_t length){
    struct Uint64Result result = {};
    if(length<3){
        result.errored = PARSENUMBER_UNEXPECTED_END;
        return result;
    }
    if(str[0] != '0'){
        result.errored = PARSENUMBER_INVALID_CHARACTER;
        return result;
    }
    if(str[1] != 'x' && str[1] != 'X'){
        result.errored = PARSENUMBER_INVALID_CHARACTER;
        return result;
    }
    return parse_hex_inner(str+2, length-2);
}

static inline warn_unused struct Uint64Result parse_binary_inner(const char*, size_t);

static inline
warn_unused
struct Uint64Result
parse_binary(const char* str, size_t length){
    struct Uint64Result result = {};
    if(length<3){
        result.errored = PARSENUMBER_UNEXPECTED_END;
        return result;
    }
    if(str[0] != '0'){
        result.errored = PARSENUMBER_INVALID_CHARACTER;
        return result;
    }
    if(str[1] != 'b' && str[1] != 'B'){
        result.errored = PARSENUMBER_INVALID_CHARACTER;
        return result;
    }
    return parse_binary_inner(str+2, length-2);
}

static inline
warn_unused
struct Uint64Result
parse_binary_inner(const char* str, size_t length){
    struct Uint64Result result = {};
    unsigned long long mask = 1llu << 63;
    mask >>= (64 - length);
    // @speed
    // 2**4 is only 16, so we could definitely
    // read 4 bytes at a time and then do a fixup.
    // You'd have to see what code is generated though
    // (does the compiler turn it into a binary decision tree?)
    // 2**8 is only 256, that's probably not worth it.
    for(size_t i = 0; i < length; i++, mask>>=1){
        switch(str[i]){
            case '1':
                result.result |= mask;
                continue;
            case '0':
                continue;
            default:
                result.errored = PARSENUMBER_INVALID_CHARACTER;
                return result;
        }
    }
    return result;
}

static inline
warn_unused
struct Uint64Result
parse_unsigned_human(const char* str, size_t length){
    struct Uint64Result result = {};
    if(!length){
        result.errored = PARSENUMBER_UNEXPECTED_END;
        return result;
    }
    if(str[0] == '#')
        return parse_pound_hex(str, length);
    if(str[0] == '0' && length > 1){
        if(str[1] == 'x' || str[1] == 'X')
            return parse_hex(str, length);
        if(str[1] == 'b' || str[1] == 'B')
            return parse_binary(str, length);
    }
    return parse_uint64(str, length);
}

#ifdef __clang__
#pragma clang assume_nonnull end
#endif

#endif
