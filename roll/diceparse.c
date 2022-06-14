//
// Copyright © 2021-2022, David Priver
//
#ifndef DICEPARSE_C
#define DICEPARSE_C
#include <stddef.h>
#include "diceparse.h"
#include "parse_numbers.h"

#ifdef __clang__
#pragma clang assume_nonnull begin
#endif
static inline void diceparse_skip_spaces(StringView* sv);

static inline char diceparse_peek(StringView* sv);

static inline void diceparse_advance(StringView* sv);

static inline int diceparse_match(StringView* sv, const char* chars);

static inline int diceparse_expralloc(DiceParseExprBuffer* buff);

static inline int diceparse_parse_comparison(DiceParseExprBuffer*, StringView*);
static inline int diceparse_parse_addplus(DiceParseExprBuffer*, StringView*);
static inline int diceparse_parse_muldiv(DiceParseExprBuffer*, StringView*);
static inline int diceparse_parse_unary(DiceParseExprBuffer*, StringView*);
static inline int diceparse_parse_terminal(DiceParseExprBuffer*, StringView*);

static inline int diceparse_make_number(DiceParseExprBuffer* buff, int n);
static inline int diceparse_make_die(DiceParseExprBuffer* buff, int n, int base);
static inline int diceparse_make_binary(DiceParseExprBuffer* buff, DiceParseBinOp op, int lhs, int rhs);


static inline
void
diceparse_skip_spaces(StringView* sv){
    for(;sv->length; sv->text++, sv->length--){
        switch(*sv->text){
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                continue;
            default:
                return;
        }
    }
}

static inline
char
diceparse_peek(StringView* sv){
    return sv->length? sv->text[0]: 0;
}


static inline
void
diceparse_advance(StringView* sv){
    sv->text++;
    sv->length--;
}

static inline
int
diceparse_match(StringView* sv, const char* chars){
    char c = diceparse_peek(sv);
    for(;*chars;chars++){
        if(*chars == c) {
            diceparse_advance(sv);
            return c;
        }
    }
    return 0;
}

static inline
int
diceparse_expralloc(DiceParseExprBuffer* buff){
    if(buff->cursor >= 1024) return -1;
    return buff->cursor++;
}

DICEPARSE_API
int
diceparse_parse(DiceParseExprBuffer* buff, StringView sv){
    int index = diceparse_parse_comparison(buff, &sv);
    if(index < 0) return index;
    diceparse_skip_spaces(&sv);
    if(sv.length != 0) return -1;
    return index;
}

static inline
int
diceparse_make_number(DiceParseExprBuffer* buff, int n){
    int result = diceparse_expralloc(buff);
    if(result < 0) return result;
    DiceParseExpr* e = &buff->exprs[result];
    e->type = DICEPARSE_NUMBER;
    e->primary = n;
    e->secondary = 1;
    return result;
}

static inline
int
diceparse_make_die(DiceParseExprBuffer* buff, int n, int base){
    int result = diceparse_expralloc(buff);
    if(result < 0) return result;
    DiceParseExpr* e = &buff->exprs[result];
    e->type = DICEPARSE_DIE;
    e->primary = base;
    e->secondary = n;
    return result;
}

static inline
int
diceparse_make_binary(DiceParseExprBuffer* buff, DiceParseBinOp op, int lhs, int rhs){
    int result = diceparse_expralloc(buff);
    if(result < 0) return result;
    DiceParseExpr* e = &buff->exprs[result];
    e->type = DICEPARSE_BINARY;
    e->type2 = op;
    e->primary = lhs;
    e->secondary = rhs;
    return result;
}


static inline
int
diceparse_parse_comparison(DiceParseExprBuffer* buff, StringView* sv){
    diceparse_skip_spaces(sv);
    int lhs = diceparse_parse_addplus(buff, sv);
    if(lhs < 0) return lhs;
    diceparse_skip_spaces(sv);
    char c;
    while((c = diceparse_match(sv, "!<>="))){
        char c2;
        c2 = diceparse_match(sv, "=");
        if(c == '!'){
            if(!c2) return -1;
        }
        int rhs = diceparse_parse_addplus(buff, sv);
        if(rhs < 0) return rhs;
        DiceParseBinOp op;
        switch(c){
            case '=': op = DICEPARSE_EQ;                                break;
            case '!': op = DICEPARSE_NOT_EQ;                            break;
            case '<': op = c2? DICEPARSE_LESS_EQ: DICEPARSE_LESS;       break;
            case '>': op = c2? DICEPARSE_GREATER_EQ: DICEPARSE_GREATER; break;
            default: return -1;
        }
        lhs = diceparse_make_binary(buff, op, lhs, rhs);
        if(lhs < 0) return lhs;
        diceparse_skip_spaces(sv);
    }
    return lhs;
}

static inline
int
diceparse_parse_addplus(DiceParseExprBuffer* buff, StringView* sv){
    diceparse_skip_spaces(sv);
    int lhs = diceparse_parse_muldiv(buff, sv);
    if(lhs < 0) return lhs;
    diceparse_skip_spaces(sv);
    char c;
    while((c=diceparse_match(sv, "+-"))){
        int rhs = diceparse_parse_muldiv(buff, sv);
        if(rhs < 0) return rhs;
        lhs = diceparse_make_binary(buff, c=='+'?DICEPARSE_ADD:DICEPARSE_SUBTRACT, lhs, rhs);
        if(lhs < 0) return lhs;
        diceparse_skip_spaces(sv);
    }
    return lhs;
}

static inline
int
diceparse_parse_muldiv(DiceParseExprBuffer* buff, StringView* sv){
    diceparse_skip_spaces(sv);
    int lhs = diceparse_parse_unary(buff, sv);
    if(lhs < 0) return lhs;
    diceparse_skip_spaces(sv);
    char c;
    while((c = diceparse_match(sv, "*/"))){
        int rhs = diceparse_parse_unary(buff, sv);
        if(rhs < 0) return rhs;
        lhs = diceparse_make_binary(buff, c=='*'?DICEPARSE_MULTIPLY:DICEPARSE_DIVIDE, lhs, rhs);
        if(lhs < 0) return lhs;
        diceparse_skip_spaces(sv);
    }
    return lhs;
}

static inline
int
diceparse_parse_unary(DiceParseExprBuffer* buff, StringView* sv){
    diceparse_skip_spaces(sv);
    char c;
    if((c = diceparse_match(sv, "+!-"))){
        int lhs = diceparse_expralloc(buff);
        if(lhs < 0) return lhs;
        int rhs = diceparse_parse_unary(buff, sv);
        if(rhs < 0) return rhs;
        DiceParseExpr* e = &buff->exprs[lhs];
        e->type = DICEPARSE_UNARY;
        switch(c){
            case '+': e->type2 = DICEPARSE_PLUS; break;
            case '!': e->type2 = DICEPARSE_NOT;  break;
            case '-': e->type2 = DICEPARSE_NEG;  break;
        }
        e->primary = rhs;
        return lhs;
    }
    return diceparse_parse_terminal(buff, sv);
}

static inline
int
diceparse_parse_terminal(DiceParseExprBuffer* buff, StringView* sv){
    diceparse_skip_spaces(sv);
    if(diceparse_match(sv, "(")){
        int lhs = diceparse_expralloc(buff);
        if(lhs < 0) return lhs;
        int rhs = diceparse_parse_comparison(buff, sv);
        if(rhs < 0) return rhs;
        diceparse_skip_spaces(sv);
        if(!diceparse_match(sv, ")")) return -1;
        DiceParseExpr* e = &buff->exprs[lhs];
        e->type = DICEPARSE_GROUPING;
        e->primary = rhs;
        return lhs;
    }
    // Don't skip spaces!
    // These are all "tight"
    if(diceparse_match(sv, "dD")){
        if(diceparse_match(sv, "%"))
            return diceparse_make_die(buff, 1, 100);
        char p = diceparse_peek(sv);
        if(p < '0' || p > '9')
            return -1;
        const char* begin = sv->text;
        const char* end = begin+1;
        diceparse_advance(sv);
        for(;;){
            p = diceparse_peek(sv);
            if(p < '0' || p > '9')
                break;
            end++;
            diceparse_advance(sv);
        }
        struct Uint64Result r = parse_uint64(begin, end-begin);
        if(r.errored) return -1;
        if(r.result > UINT16_MAX) return -1;
        return diceparse_make_die(buff, 1, (int)r.result);
    }
    char p = diceparse_peek(sv);
    if(p < '0' || p > '9')
        return -1;
    const char* begin = sv->text;
    const char* end = begin+1;
    diceparse_advance(sv);
    for(;;){
        p = diceparse_peek(sv);
        if(p < '0' || p > '9')
            break;
        end++;
        diceparse_advance(sv);
    }
    uint32_t val;
    {
        struct Uint64Result r = parse_uint64(begin, end-begin);
        if(r.errored) return -1;
        if(r.result > UINT32_MAX) return -1;
        val = (uint32_t)r.result;
    }
    if(!diceparse_match(sv, "dD"))
        return diceparse_make_number(buff, val);
    if(diceparse_match(sv, "%")){
        if(val > UINT16_MAX) return -1;
        return diceparse_make_die(buff, val, 100);
    }
    p = diceparse_peek(sv);
    if(p < '0' || p > '9')
        return -1;
    begin = sv->text;
    end = begin+1;
    diceparse_advance(sv);
    for(;;){
        p = diceparse_peek(sv);
        if(p < '0' || p > '9')
            break;
        end++;
        diceparse_advance(sv);
    }
    int val2;
    {
        struct Uint64Result r = parse_uint64(begin, end-begin);
        if(r.errored) return -1;
        if(r.result > UINT16_MAX) return -1;
        val2 = r.result;
    }
    if(val > UINT16_MAX) return -1;
    return diceparse_make_die(buff, val, val2);
}

#ifdef __clang__
#pragma clang assume_nonnull end
#endif

#endif
