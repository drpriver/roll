//
// Copyright © 2021-2022, David Priver
//
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifndef _WIN32
#include <unistd.h>
static inline int stdin_is_interactive(void){
    return isatty(STDIN_FILENO);
}
#else
#include <io.h>
static inline int stdin_is_interactive(void){
    return _isatty(0);
}
#endif
#include "common_macros.h"
#include "rng.h"
#include "long_string.h"
#include "StringBuilder.h"
#include "get_input.h"
#include "argument_parsing.h"
#include "diceparse.h"

static struct LineHistory history;

#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

static
int64_t
validate_inner(DiceParseExpr* exprs, DiceParseExpr expr);

static
bool
validate(DiceParseExpr* exprs, DiceParseExpr expr){
    int64_t biggest_value = validate_inner(exprs, expr);
    return biggest_value >= 0 && biggest_value <= INT64_MAX;
}

static
int64_t
validate_inner(DiceParseExpr* exprs, DiceParseExpr expr){
    switch((DiceParseExpressionType)expr.type){
        case DICEPARSE_NUMBER:
            return expr.primary;
        case DICEPARSE_DIE:
            if(expr.primary == 0 || expr.secondary == 0)
                return 0;
            // dice are limited to uint16, so we can safely multiply
            return (int64_t)expr.primary * (int64_t)expr.secondary;
        case DICEPARSE_BINARY:{
            int64_t lhs = validate_inner(exprs, exprs[expr.primary]);
            if(lhs < 0) return lhs;
            int64_t rhs = validate_inner(exprs, exprs[expr.secondary]);
            if(rhs < 0) return rhs;
            int64_t result;
            switch((DiceParseBinOp)expr.type2){
                case DICEPARSE_ADD:
                    if(__builtin_add_overflow(lhs, rhs, &result))
                        return -1;
                    return result;
                // this might be overly conservative
                case DICEPARSE_SUBTRACT:
                    if(__builtin_add_overflow(lhs, rhs, &result))
                        return -1;
                    return result;
                case DICEPARSE_MULTIPLY:
                    if(__builtin_mul_overflow(lhs, rhs, &result))
                        return -1;
                    return result;
                case DICEPARSE_DIVIDE:
                    if(!rhs) return 0;
                    // Hmm. we need an actual min to do this one.
                    return lhs / 1;
                case DICEPARSE_EQ:
                    return 1;
                case DICEPARSE_NOT_EQ:
                    return 1;
                case DICEPARSE_LESS:
                    return 1;
                case DICEPARSE_LESS_EQ:
                    return 1;
                case DICEPARSE_GREATER:
                    return 1;
                case DICEPARSE_GREATER_EQ:
                    return 1;
            }
        }
        case DICEPARSE_UNARY:{
            int64_t rhs = validate_inner(exprs, exprs[expr.primary]);
            if(rhs < 0) return rhs;
            switch((DiceParseUnaryOp)expr.type2){
                case DICEPARSE_PLUS:
                    return rhs;
                case DICEPARSE_NEG:
                    return rhs;
                case DICEPARSE_NOT:
                    return 1;
            }
        }
        case DICEPARSE_GROUPING:
            return validate_inner(exprs, exprs[expr.primary]);
    }
}

static
int64_t
roll_and_display(DiceParseExpr* exprs, DiceParseExpr expr, RngState* rng, bool verbose, bool tight){
    #define max_coloring "\033[92m"
    #define min_coloring "\033[91m"
    #define reset_coloring "\033[39;49m"
    switch((DiceParseExpressionType)expr.type){
        case DICEPARSE_NUMBER:
            if(verbose)printf("%d", (int)expr.primary);
            return expr.primary;
        case DICEPARSE_DIE:{
            if(expr.secondary == 0 || expr.primary == 0){
                if(verbose)printf("[0]");
                return 0;
            }
            if(expr.secondary == 1){
                int64_t num = bounded_random(rng, expr.primary) + 1;
                if(verbose){
                    const char* color = "";
                    if(num == expr.primary) 
                        color = max_coloring;
                    else if(num == 1) 
                        color = min_coloring;
                    printf("[%s%lld%s]", color, (long long)num, reset_coloring);
                }
                return num;
            }
            if(verbose)if(tight)putchar('(');
            int64_t val = 0;
            for(int i = 0; i < expr.secondary; i++){
                if(i != 0)
                    if(verbose)putchar('+');
                int64_t num = bounded_random(rng, expr.primary) + 1;
                if(verbose){
                    const char* color = "";
                    if(num == expr.primary) 
                        color = max_coloring;
                    else if(num == 1) 
                        color = min_coloring;
                    printf("[%s%lld%s]", color, (long long)num, reset_coloring);
                }
                val += num;
            }
            if(verbose)if(tight)putchar(')');
            return val;
        }
        case DICEPARSE_BINARY:{
            int64_t lhs;
            int64_t rhs;
            switch((DiceParseBinOp)expr.type2){
                case DICEPARSE_ADD:
                    lhs = roll_and_display(exprs, exprs[expr.primary], rng, verbose, false);
                    if(verbose)printf(" + ");
                    rhs = roll_and_display(exprs, exprs[expr.secondary], rng, verbose, false);
                    return lhs + rhs;
                case DICEPARSE_SUBTRACT:
                    lhs = roll_and_display(exprs, exprs[expr.primary], rng, verbose, false);
                    if(verbose)printf(" - ");
                    rhs = roll_and_display(exprs, exprs[expr.secondary], rng, verbose, false);
                    return lhs - rhs;
                case DICEPARSE_MULTIPLY:
                    lhs = roll_and_display(exprs, exprs[expr.primary], rng, verbose, true);
                    if(verbose)putchar('*');
                    rhs = roll_and_display(exprs, exprs[expr.secondary], rng, verbose, true);
                    return lhs * rhs;
                case DICEPARSE_DIVIDE:
                    lhs = roll_and_display(exprs, exprs[expr.primary], rng, verbose, true);
                    if(verbose)putchar('/');
                    rhs = roll_and_display(exprs, exprs[expr.secondary], rng, verbose, true);
                    if(!rhs) return 0;
                    return lhs / rhs;
                case DICEPARSE_EQ:
                    lhs = roll_and_display(exprs, exprs[expr.primary], rng, verbose, false);
                    if(verbose)printf(" = ");
                    rhs = roll_and_display(exprs, exprs[expr.secondary], rng, verbose, false);
                    return lhs == rhs;
                case DICEPARSE_NOT_EQ:
                    lhs = roll_and_display(exprs, exprs[expr.primary], rng, verbose, false);
                    if(verbose)printf(" != ");
                    rhs = roll_and_display(exprs, exprs[expr.secondary], rng, verbose, false);
                    return lhs != rhs;
                case DICEPARSE_LESS:
                    lhs = roll_and_display(exprs, exprs[expr.primary], rng, verbose, false);
                    if(verbose)printf(" < ");
                    rhs = roll_and_display(exprs, exprs[expr.secondary], rng, verbose, false);
                    return lhs < rhs;
                case DICEPARSE_LESS_EQ:
                    lhs = roll_and_display(exprs, exprs[expr.primary], rng, verbose, false);
                    if(verbose)printf(" <= ");
                    rhs = roll_and_display(exprs, exprs[expr.secondary], rng, verbose, false);
                    return lhs <= rhs;
                case DICEPARSE_GREATER:
                    lhs = roll_and_display(exprs, exprs[expr.primary], rng, verbose, false);
                    if(verbose)printf(" > ");
                    rhs = roll_and_display(exprs, exprs[expr.secondary], rng, verbose, false);
                    return lhs > rhs;
                case DICEPARSE_GREATER_EQ:
                    lhs = roll_and_display(exprs, exprs[expr.primary], rng, verbose, false);
                    if(verbose)printf(" >= ");
                    rhs = roll_and_display(exprs, exprs[expr.secondary], rng, verbose, false);
                    return lhs >= rhs;
            }
        }
        case DICEPARSE_GROUPING:{
            if(verbose)putchar('(');
            int64_t val = roll_and_display(exprs, exprs[expr.primary], rng, verbose, false);
            if(verbose)putchar(')');
            return val;
        }
        case DICEPARSE_UNARY:{
            int64_t val;
            switch((DiceParseUnaryOp)expr.type2){
                case DICEPARSE_PLUS:
                    if(verbose)putchar('+');
                    val = roll_and_display(exprs, exprs[expr.primary], rng, verbose, tight);
                    return val;
                case DICEPARSE_NEG:
                    if(verbose)putchar('-');
                    val = -roll_and_display(exprs, exprs[expr.primary], rng, verbose, true);
                    return val;
                case DICEPARSE_NOT:
                    if(verbose)putchar('!');
                    val = !roll_and_display(exprs, exprs[expr.primary], rng, verbose, true);
                    return val;
            }
        }
    }
}
static
void
interactive_mode(bool verbose) {
    puts("ctrl-d or \"q\" to exit");
    puts("\"v\" toggles verbose output");
    puts("Enter repeats last die roll");
    enum {INPUT_SIZE=1024};
    char inp[INPUT_SIZE];
    RngState rng = {0};
    seed_rng_auto(&rng);
    LongString prompt = {.length = sizeof(">> ")-1, .text=">> "};
    DiceParseExprBuffer buff = {0};
    for(ssize_t err_or_len = get_input_line(&history, prompt, inp, INPUT_SIZE);err_or_len >= 0; err_or_len = get_input_line(&history, prompt, inp, INPUT_SIZE)){
        LongString input = {.length = err_or_len, .text=inp};
        if(input.text[0] == 'q')
            break;
        if(input.text[0] == 'v'){
            verbose = !verbose;
            continue;
        }
        if(!input.length){
            fputs("\033[F", stdout);
            fflush(stdout);
            if(history.count)
                input = history.history[history.count-1];
            else 
                continue;
        }
        else {
            putchar('\r');
        }
        int index = diceparse_parse(&buff, LS_to_SV(input));
        if(index < 0){
            fputs("Error when parsing dice expression.\n", stdout);
            continue;
        }
        if(!validate(buff.exprs, buff.exprs[index])){
            fputs("Error: would overflow\n", stdout);
            continue;
        }
        add_line_to_history(&history, input);
        int64_t val = roll_and_display(buff.exprs, buff.exprs[index], &rng, verbose, false);
        printf(" -> %lld\n", val);
    }
    puts("");
    return;
}

int 
main(int argc, const char** argv) {

    bool verbose = stdin_is_interactive();
    ArgToParse kw_args[] = {
        {
            .name = SV("-v"),
            .altname1 = SV("--verbose"),
            .help = "Display the individual dice rolls instead of just the total.",
            .max_num = 1,
            .dest = ARGDEST(&verbose),
        },
    };
    StringView dice_strings[64];
    ArgToParse pos_args[] = {
        {
            .name = SV("dice"),
            .help = "The dice expression to roll.",
            .max_num = arrlen(dice_strings),
            .dest = ARGDEST(&dice_strings[0]),
        },
    };
    enum {HELP};
    ArgToParse early_out_args[] = {
        [HELP] = {
            .name = SV("-h"),
            .altname1 = SV("--help"),
            .help = "Print this help and exit.",
        }
    };
    ArgParser parser = {
        .name             = argv[0],
        .early_out.args   = early_out_args,
        .early_out.count  = arrlen(early_out_args),
        .positional.args  = pos_args,
        .positional.count = arrlen(pos_args),
        .keyword.args     = kw_args,
        .keyword.count    = arrlen(kw_args),
        .description      = "A program for rolling dice.",
    };
    Args args = {argc-1, argv+1};
    switch(check_for_early_out_args(&parser, &args)){
        case HELP:{
            int columns = get_cols();
            if(columns > 80)
                columns = 80;
            print_argparse_help(&parser, columns);
            return 0;
        }
        default:
            break;
    }
    auto parse_e = parse_args(&parser, &args, ARGPARSE_FLAGS_UNKNOWN_KWARGS_AS_ARGS);
    if(parse_e){
        print_argparse_error(&parser, parse_e);
        return parse_e;
    }

    if(pos_args[0].num_parsed < 1){
        if(stdin_is_interactive()){
            load_history(&history);
            interactive_mode(verbose);
            dump_history(&history);
        }
        else {
            char buff[4192];
            RngState rng = {0};
            seed_rng_auto(&rng);
            DiceParseExprBuffer exprbuffer = {0};
            while(fgets(buff, sizeof(buff), stdin)){
                size_t length = strlen(buff);
                buff[--length] = '\0';
                StringView input = {.text = buff, .length = length};
                if(input.length == 1 && input.text[0] == 'v'){
                    verbose = !verbose;
                    continue;
                }
                int index = diceparse_parse(&exprbuffer, input);
                if(index < 0)
                    return 1;
                if(!validate(exprbuffer.exprs, exprbuffer.exprs[index]))
                    return 1;
                int64_t value = roll_and_display(exprbuffer.exprs, exprbuffer.exprs[index], &rng, verbose, false);
                printf("%s%lld\n", verbose?" -> ":"", value);
            }
        }
        return 0;
    }
    StringBuilder sb = {0};
    for(int i = 0; i < pos_args[0].num_parsed; i++){
        sb_write_str(&sb, " ", 1);
        sb_write_str(&sb, dice_strings[i].text, dice_strings[i].length);
    }
    LongString input = sb_borrow(&sb);
    DiceParseExprBuffer exprbuffer = {0};
    RngState rng = {0};
    seed_rng_auto(&rng);
    int index = diceparse_parse(&exprbuffer, LS_to_SV(input));
    if(index < 0)
        return 1;
    if(!validate(exprbuffer.exprs, exprbuffer.exprs[index]))
        return 1;
    int64_t value = roll_and_display(exprbuffer.exprs, exprbuffer.exprs[index], &rng, verbose, false);
    printf("%s%lld\n", verbose?" -> ":"", value);
    return 0;
}

#ifdef __clang__
#pragma clang assume_nonnull end
#endif

#include "get_input.c"
#include "diceparse.c"
