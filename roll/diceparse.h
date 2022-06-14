//
// Copyright © 2021-2022, David Priver
//
#ifndef DICEPARSE_H
#define DICEPARSE_H
#include <stdint.h>
#include "long_string.h"

#ifndef DICEPARSE_API
#define DICEPARSE_API extern
#endif

#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

typedef struct DiceParseExpr {
    // This is an ExpressionType
    uint8_t type;
    // For BINARY, this is primary BinOp
    // For UNARY, the UnaryOp
    uint8_t type2;
    // For DIE, this is the number of dice.
    // For BINARY, this is the index of the rhs.
    uint16_t secondary;
    // For NUMBER, this is the value of the expression
    // For DIE, this is the base of the dice.
    //    The value for dice will be restricted to 16 bits.
    // For BINARY, the index of the lhs expression
    // For GROUPING, the index of the expression
    // For UNARY, the index of the expression the op is applied to
    uint32_t primary;
} DiceParseExpr;
_Static_assert(sizeof(struct DiceParseExpr) == 8, "");

typedef struct DiceParseExprBuffer {
    DiceParseExpr exprs[1024];
    int cursor;
} DiceParseExprBuffer;

typedef enum DiceParseExpressionType {
    DICEPARSE_NUMBER,
    DICEPARSE_DIE,
    DICEPARSE_BINARY,
    DICEPARSE_GROUPING,
    DICEPARSE_UNARY,
} DiceParseExpressionType;

typedef enum DiceParseBinOp {
    DICEPARSE_ADD,
    DICEPARSE_SUBTRACT,
    DICEPARSE_MULTIPLY,
    DICEPARSE_DIVIDE,
    DICEPARSE_EQ,
    DICEPARSE_NOT_EQ,
    DICEPARSE_LESS,
    DICEPARSE_LESS_EQ,
    DICEPARSE_GREATER,
    DICEPARSE_GREATER_EQ,
} DiceParseBinOp;

typedef enum DiceParseUnaryOp {
    DICEPARSE_PLUS,
    DICEPARSE_NEG,
    DICEPARSE_NOT,
} DiceParseUnaryOp;

DICEPARSE_API
int
diceparse_parse(DiceParseExprBuffer* buff, StringView sv);

#ifdef __clang__
#pragma clang assume_nonnull end
#endif

#endif
