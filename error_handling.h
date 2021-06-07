#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H
// size_t
#include <stddef.h>
#include "david_macros.h"
#define ERROR_CODES(apply) \
    apply(NO_ERROR, 0) \
    /*Messages*/ \
    apply(QUIT, 1) \
    apply(BAD_EXIT, 2) \
    /*Container errors*/ \
    apply(NOT_FOUND, 3) \
    apply(OUT_OF_SPACE, 4) \
    /*Parser Errors*/ \
    apply(PARSE_ERROR, 5) \
    apply(INVALID_SYMBOL, 6) \
    apply(UNEXPECTED_END, 7) \
    apply(INFIX_MISSING, 8) \
    /*Buffer Errors*/ \
    apply(UNDERFLOWED_BUFFER, 9) \
    apply(OVERFLOWED_BUFFER, 10) \
    apply(WOULD_OVERFLOW, 11) \
    /* decoding error */ \
    apply(ENCODING_ERROR, 12)\
    apply(DECODING_ERROR, 13)\
    /*Math Errors*/ \
    apply(INVALID_VALUE, 14) \
    apply(OVERFLOWED_VALUE, 15) \
    /*Dice Errors?*/ \
    apply(EXCESSIVE_DICE, 16) \
    /*File IO Errors*/ \
    apply(FILE_ERROR, 17) \
    apply(FILE_NOT_OPENED, 18) \
    apply(FILE_NOT_FOUND, 19) \
    apply(FILE_CREATE_FAILED, 20) \
    apply(NO_BYTES_WRITTEN, 21) \
    apply(NO_BYTES_READ, 22) \
    apply(INVALID_FILE_TYPE, 23) \
    apply(ALREADY_EXISTS, 24) \
    /*Pointer Errors*/ \
    apply(UNEXPECTED_NULL, 25) \
    apply(ALLOC_FAILURE, 26) \
    apply(DEALLOC_FAILURE, 27) \
    /*Initialization Errors*/ \
    apply(ALREADY_INIT, 28) \
    apply(FAILED_INIT, 29) \
    /*SDL Errors*/ \
    apply(SDL_ERROR, 30) \
    /*Combat Error?*/ \
    apply(UNREACHABLE, 31) \
    /*KWARG*/ \
    apply(BAD_KWARG, 32) \
    apply(MISSING_KWARG, 33) \
    apply(EXCESS_KWARGS, 34) \
    apply(DUPLICATE_KWARG, 35) \
    apply(MISSING_ARG, 36) \
    apply(EARLY_END, 37) \
    /*Some low level routine failed*/ \
    apply(OS_ERROR, 38) \
    /*idk man*/ \
    apply(GENERIC_ERROR, 39)\

typedef enum ErrorCode {
#define X(x, v) x = v,
    ERROR_CODES(X)
#undef X
} ErrorCode;

#define _Errorable_impl(T) T##__Errorable
#define Errorable(T) struct _Errorable_impl(T)
#define Errorable_f(T) warn_unused struct _Errorable_impl(T)
#define Errorable_declare(T) Errorable(T) { T result; ErrorCode errored; }
//Declare some common types
struct _Errorable_impl(void) {
    ErrorCode errored;
};
Errorable_declare(int);
Errorable_declare(char);
#define Raise(error_value) ({result.errored = error_value; return result;})

#define attempt(maybe) ({   auto const maybe_ = maybe;\
                            if(unlikely(maybe_.errored)) {\
                                result.errored = maybe_.errored;\
                                return result;\
                                }\
                            maybe_.result;})
#define attempt_void(maybe) ({ auto const maybe_ = maybe;\
                            if(unlikely(maybe_.errored)) {\
                                result.errored = maybe_.errored;\
                                return result;\
                                }\
                            ;})
#endif
