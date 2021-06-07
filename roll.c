#define ANSI_CODES
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "david_macros.h"
#include "error_handling.h"
#include "rng.h"
#include "slice.h"
#include "long_string.h"
#include "StringBuilder.h"
typedef struct Die {
    int base;
    int count;
    } Die;
Errorable_declare(Die);
SliceDeclare(Die);
SliceDeclareImpl(Die);
typedef Slice(Die) DiceExpression;

static
Errorable_f(void)
dice_canonicalize(Nonnull(DiceExpression*) de){
    Errorable(void) result = {};
    for(int i = 1; i < de->count; i++){
        auto d = &de->data[i];
        if(d->count == 0){
            Slice_remove_at(Die)(de, i);
            i--;
            continue;
            }
        for(int j = 0; j < i; j++){
            if(de->data[j].base == d->base){
                // TODO: handle overflow
                // In theory, we can just add more dice
                if(__builtin_add_overflow(de->data[j].count, d->count, &de->data[j].count))
                    Raise(OVERFLOWED_VALUE);
                Slice_remove_at(Die)(de, i);
                i--;
                break;
                }
            }
        }
    return result;
    }

static
void
print_dice_expr(DiceExpression de) {
    int printed = 0;
    for (int i = 0; i < de.count; i++) {
        bool negative = false;
        auto die = de.data[i];
        auto count = die.count;
        auto base  = die.base;
        if (base == 0 or count == 0)
            continue;
        if (die.count < 0) {
            negative = !negative;
            count = -count;
            }
        if (die.base < 0)  {
            negative = !negative;
            base = -base;
            }
        if (negative)
            printed += !!putchar('-');
        else if (printed > 0)
            printed += !!putchar('+');

        if (base == 1) {
            printed += printf("%d", count);
            continue;
            }

        if (count > 1)
            printed += printf("%d", count);

        printed += !!putchar('d');
        printed += printf("%d", base);
        }
    if(printed == 0)
        putchar('0');
    return;
    }

static
int64_t
roll_die(Die die, Nonnull(RngState*) rng) {
    if (die.base == 0)
        return 0;
    if (die.base == 1)
        return die.count;
    int result = 0;
    auto count = die.count;
    if (die.count < 0)
        count = -count;
    else if (die.count == 0)
        return 0;
    for (int i = 0; i < count; ++i) {
        auto base = die.base;
        if (base < 0)
            result -= bounded_random(rng, -base)+1;
        else
            result += bounded_random(rng, base)+1;
        }
    if (die.count < 0 )
        return -result;
    else
        return result;
    }

static
int64_t
roll_dice(DiceExpression dice, Nonnull(RngState*) rng) {
    int64_t result = 0;
    for (size_t i = 0; i < dice.count; ++i) {
        result += roll_die(dice.data[i], rng);
        }
    return result;
    }

SliceDeclare(char);
SliceDeclareImpl(char);

static
Errorable_f(void)
push_char(Nonnull(Slice(char)*) stack, char c) {
    Errorable(void) result = {};
    if (stack->count >= stack->capacity) {
        Raise(OVERFLOWED_BUFFER);
        }
    Slice_append(char)(stack, c);
    return result;
    }

static
Errorable_f(char)
pop_char(Nonnull(Slice(char)*) stack) {
    Errorable(char) result = {};
    if (stack->count == 0 ) Raise(UNDERFLOWED_BUFFER);
    result.result = Slice_pop(char)(stack);
    return result;
    }

static
Errorable_f(int)
magnitude_char(char c, int pow) {
    Errorable(int) result = {};
    int base = c - '0';
    if (base > 9 or base < 0) {
        Raise(INVALID_SYMBOL);
        }
    if (pow < 0) {
        Raise(INVALID_VALUE);
        }
    result.result = base;
    _Static_assert(sizeof(int) > sizeof(int16_t), "Overflow check is invalid");
    for(int rep = 1; rep < pow; rep++) {
        result.result *= 10;
        if (result.result > INT16_MAX)
            Raise(OVERFLOWED_VALUE);
        }
    return result;
    }

static
Errorable_f(int)
integer_from_stack(Nonnull(Slice(char)*) stack) {
    int pow = 0;
    int total = 0;
    Errorable(int) result = {};
    char c = attempt(pop_char(stack));
    while(1) {
        pow++;
        total += attempt(magnitude_char(c, pow));
        auto d = pop_char(stack);
        if(d.errored) break;
        c = d.result;
        }
    result.result = total;
    return result;
    }

static
Errorable_f(Die)
constant_die(int number) {
    Errorable(Die) result = {};
    if (number > INT16_MAX) {
        Raise(OVERFLOWED_VALUE);
        }
    result.result = (Die){.count = number, .base=1};
    return result;
    }

static
Errorable_f(Die)
var_dice(int count, int base) {
    Errorable(Die) result = {};
    if (count > INT16_MAX or base > INT16_MAX) {
        Raise(OVERFLOWED_VALUE);
        }
    result.result = (Die){.count = count, .base=base};
    return result;
    }

struct parse_iterator{
    Nonnull(char*) const text;
    size_t current_position;
    bool stop;
    int dice_count;
    };

static
struct parse_iterator
start_parse(LongString input) {
    size_t position = 0;
    for(;position < input.length; position++) {
        if(position==0){
            if (input.text[position] != '/') break;
            else continue;
            }
        if(position==1){
            if(input.text[position] != 'r') break;
            else continue;
            }
        if(position==2){
            if(input.text[position] != 'o') break;
            else continue;
            }
        if(position==3 or position==4) {
            if(input.text[position] != 'l') break;
            else continue;
            }
        break;
        }
    struct parse_iterator iter = {
        .text = input.text,
        .current_position = position,
        .stop = 0,
        .dice_count = 0
        };
    return iter;
    }
enum {BUFFER_SIZE=16};
static
Errorable_f(Die)
next_parse(Nonnull(struct parse_iterator*) iter) {
    Errorable(Die) result = {};
    if(iter->stop)
        return result;
    iter->dice_count++;
    bool negative = false;
    bool infix_symbol = false;
    int count = 0;
    char buffer[BUFFER_SIZE];
    Slice(char) digit_chars = {
        .data = buffer,
        .count = 0,
        .capacity = BUFFER_SIZE,
        };
    //handle leading symbols
    for(char c = iter->text[iter->current_position];
        ;
        iter->current_position++, c = iter->text[iter->current_position]) {
        switch(c) {
            case ' ':
                continue;
            case '+':
                infix_symbol=true;
                continue;
            case '-':
                infix_symbol=true;
                negative = !negative;
                continue;
            case '\0':
            case '\n':
                if(infix_symbol) {
                    iter->stop=true;
                    Raise(UNEXPECTED_END);
                    }
                iter->stop = true;
                return result;
            case 'd':
            case 'D':
                if(infix_symbol or iter->dice_count==1) {
                    count = 1;
                    iter->current_position++;
                    goto parse_base;
                    }
                else {
                    Raise(INFIX_MISSING);
                    }
            case '0' ... '9':
                if (infix_symbol or iter->dice_count==1) {
                    goto parse_count;
                    }
                else {
                    Raise(INFIX_MISSING);
                    }

            default:
                iter->stop=true;
                Raise(INVALID_SYMBOL);
            }
        }
    parse_count:;
    int ci = 0;
    for(char c = iter->text[iter->current_position];
        ;
        iter->current_position++, c = iter->text[iter->current_position]) {
        switch(c) {
            case ' ':
                count = attempt(integer_from_stack(&digit_chars));
                result.result = attempt(constant_die(count));
                if (negative) {
                    result.result.count *= -1;
                    }
                return result;
            case '+':
            case '-':
                count = attempt(integer_from_stack(&digit_chars));
                result.result = attempt(constant_die(count));
                if (negative) {
                    result.result.count *= -1;
                    }
                return result;
            case '\0':
            case '\n':
                iter->stop = true;
                if (ci > 0 ) {
                    count = attempt(integer_from_stack(&digit_chars));
                    result.result = attempt(constant_die(count));
                    if (negative) {
                        result.result.count *= -1;
                        }
                    }
                return result;
            case '0' ... '9':
                attempt_void(push_char(&digit_chars, c));
                ci++;
                continue;
            case 'd':
            case 'D':
                iter->current_position++;
                count = attempt(integer_from_stack(&digit_chars));
                goto parse_base;
            default:
                iter->stop = true;
                Raise(INVALID_SYMBOL);
            }
        }
    parse_base:;
    int bi = 0;
    int base = 0;
    for (   char c = iter->text[iter->current_position];
            ;
            iter->current_position++, c = iter->text[iter->current_position]) {
        switch(c) {
            case '\0':
            case '\n':
                iter->stop=true;
                goto end_counting;
            case '+':
            case '-':
            case ' ':
                goto end_counting;
            case '0' ... '9':
                attempt_void(push_char(&digit_chars, c));
                bi++;
                continue;
            default:
                iter->stop = true;
                Raise(INVALID_SYMBOL);
            }
        }
    end_counting:;
    if(bi == 0) {
        iter->stop = true;
        Raise(UNEXPECTED_END);
        }
    base = attempt(integer_from_stack(&digit_chars));
    result.result = attempt(var_dice(count, base));
    if (negative) {
        result.result.base *= -1;
        }
    return result;
    }

static
Errorable_f(void)
check_overflow(Nonnull(DiceExpression*)de){
    Errorable(void) result = {};
    int64_t max_value = 0;
    for(int i = 0; i < de->count; i++){
        auto d = de->data[i];
        if(d.base <= 0)
            continue;
        if(d.base == 1 and d.count <= 0)
            continue;
        int64_t sub_max = 0;
        if(__builtin_mul_overflow(d.base, d.count, &sub_max)){
            Raise(WOULD_OVERFLOW);
            }
        if(__builtin_add_overflow(sub_max, max_value, &max_value)){
            Raise(WOULD_OVERFLOW);
            }
        }
    int64_t min_value = 0;
    for(int i = 0; i < de->count; i++){
        auto d = de->data[i];
        if(d.base >= 0 and not (d.base==1 and d.count < 0) )
            continue;
        int64_t sub_min = 0;
        if(__builtin_mul_overflow(d.base, d.count, &sub_min)){
            Raise(WOULD_OVERFLOW);
            }
        if(__builtin_add_overflow(sub_min, min_value, &min_value)){
            Raise(WOULD_OVERFLOW);
            }
        }
    return result;
    }


enum {MAX_DICE = 16};
static
Errorable_f(void)
parse_dice_expression(LongString input, Nonnull(DiceExpression*) de) {
    Errorable(void) result = {};
    int slots_remaining = MAX_DICE;
    de->count = 0;
    auto iter = start_parse(input);
    for (Die d = attempt(next_parse(&iter));; d = attempt(next_parse(&iter))){
        if(slots_remaining <= 0) {
            Raise(EXCESSIVE_DICE);
            }
        if(!d.base and !d.count) {
            if(iter.stop)
                break;
            continue;
            }
        slots_remaining--;
        Slice_append(Die)(de, d);
        if(iter.stop) break;
        }
    if(slots_remaining==MAX_DICE) {
        de->data[0] = (Die) {};
        }
    attempt_void(dice_canonicalize(de));
    attempt_void(check_overflow(de));
    return result;
    }

static
void
report_error(ErrorCode e) {
    fprintf(stderr, "   Error in parsing: ");
    switch(e) {
        case INVALID_SYMBOL:
            fprintf(stderr, "Invalid Symbol\n");
            break;
        case UNEXPECTED_END:
            fprintf(stderr, "Unexpected End of Text Stream\n");
            break;
        case UNDERFLOWED_BUFFER:
            fprintf(stderr, "Underflowed Buffer\n");
            break;
        case OVERFLOWED_BUFFER:
            fprintf(stderr, "Single Number Too Long (exceeded %d digits)\n",BUFFER_SIZE);
            break;
        case INVALID_VALUE:
            fprintf(stderr, "Invalid Value\n");
            break;
        case OVERFLOWED_VALUE:
            fprintf(stderr, "Single Number Too Big (Larger than %d)\n", INT16_MAX);
            break;
        case EXCESSIVE_DICE:
            fprintf(stderr, "Too Many Dice in Input ");
            fprintf(stderr, "(Exceeded %d)\n", MAX_DICE);
            break;
        case INFIX_MISSING:
            fprintf(stderr, "Encountered New Dice Token Before Infix Operator (a + or -)\n");
            break;
        case WOULD_OVERFLOW:
            fprintf(stderr, "Maximum or minimum result of dice does not fit in signed 64 bits\n");
            break;
        default:
            assert(0);
            unreachable();
        }
    return;
    }
static
void
roll_and_display(DiceExpression de, Nonnull(RngState*) rng) {
    auto value = roll_dice(de, rng);
    fputs("   ", stdout);
    print_dice_expr(de);
    printf(" -> %lld\n", (long long)value);
    }
static
void
verbose_roll_and_display(DiceExpression de, Nonnull(RngState*) rng) {
    int sum = 0;
    fputs("   ", stdout);
    print_dice_expr(de);
    fputs(" -> ", stdout);
    int max = 0;
    bool can_be_negative = false;
    if(!de.count)
        putchar('0');
    for(int i = 0; i < de.count; i++) {
        if(i > 0) {
            putchar('+');
            }
        auto die = de.data[i];
        auto count = die.count;
        auto base = die.base;
        max += Abs(base) * Abs(count);
        int width = 0;
        for(auto scratch_base = base; scratch_base; scratch_base/=10) {
            width++;
            }
        if(base == 0 or count == 0) {
            putchar('0');
            continue;
            }
        if(base == 1) {
            printf("%d", count);
            sum+=count;
            continue;
            }
        if(count < 0 ) {
            count = -count;
            base = -base;
            }
        for(int j = 0; j < count; j++) {
            if(j > 0) {
                putchar('+');
                }
            auto roll = roll_die((Die) {.count=1, .base=base}, rng);
            bool rolled_max = false;
            bool rolled_min = false;
            if(Abs(base) == Abs(roll))
                rolled_max = true;
            else if (Abs(roll) == 1)
                rolled_min = true;
            sum+=roll;
            #ifdef ANSI_CODES
                #define max_coloring "\033[92m"
                #define min_coloring "\033[91m"
                #define reset_coloring "\033[39;49m"
            #else
                #define max_coloring ""
                #define min_coloring ""
                #define reset_coloring ""
            #endif
            if(rolled_max)
                fputs(max_coloring, stdout);
            else if(rolled_min)
                fputs(min_coloring, stdout);

            if (roll < 0) {
                printf("[%*lld]", width+1, (long long)roll);
                can_be_negative=true;
                }
            else
                printf("[%*lld]", width,  (long long)roll);
            if(rolled_min or rolled_max)
                fputs(reset_coloring, stdout);
            }
        }
    int final_width = 0;
    for(auto scratch_max = max; scratch_max;scratch_max/=10) {
        final_width++;
        }
    fputs(" -> ", stdout);
    if (can_be_negative)
        printf("%*d\n", final_width+1, sum);
    else
        printf("%*d\n", final_width, sum);
    }

static
Errorable_f(void)
interactive_mode(void) {
    Errorable(void) result = {};
    puts("ctrl-d or \"q\" to exit");
    puts("\"v\" toggles verbose output");
    puts("\"s\" + anyinput to seed rng");
    puts("\"sh\" + list of things to shuffle the things (' ', ',', ';') are delims)");
    puts("Enter repeats last die roll");
    enum {INPUT_SIZE=1024};
    char* inp = malloc(INPUT_SIZE);
    DiceExpression de  = {.data=alloca(sizeof(Die) * MAX_DICE), .count=0, .capacity=MAX_DICE};
    DiceExpression de2 = {.data=alloca(sizeof(Die) * MAX_DICE), .count=0, .capacity = MAX_DICE};
    auto verbose = false;
    StringBuilder sb = {};
    RngState rng_ = {};
    auto rng = &rng_;
    seed_rng(rng);
    fputs(">> ", stdout);
    fflush(stdout);
    for (;
        fgets(inp, INPUT_SIZE, stdin);
        fputs(">> ", stdout), fflush(stdout)) {
        sb_reset(&sb);
        sb_write_cstr(&sb, inp);
        sb_strip(&sb);
        auto input = sb_borrow(&sb);
        if(input.text[0] == 'q' and input.length == 1)
            break;
        if(input.text[0] == 'v' and input.length == 1){
            verbose = !verbose;
            continue;
            }
        if(!input.length){
            #ifdef ANSI_CODES
                fputs("\033[F", stdout);
                fflush(stdout);
            #else
                ;
            #endif
            }
        else {
            auto parsed_expression = parse_dice_expression(input, &de2);
            if(parsed_expression.errored) {
                report_error(parsed_expression.errored);
                continue;
                }
            else
                Swap(de, de2);
            }
        if(verbose)
            verbose_roll_and_display(de, rng);
        else
            roll_and_display(de, rng);
        }
    puts("");
    sb_destroy(&sb);
    free(inp);
    return result;
    }

int main(int argc, const char** argv) {
    if(argc < 2){
        auto e = interactive_mode().errored;
        if (e){
            report_error(e);
            return e;
            }
        return 0;
        }
    const int input_count = argc-1;
    if(input_count > 64) {
        report_error(EXCESSIVE_DICE);
        return 64;
        }
    StringBuilder sb = {};
    sb_join(&sb, " ", argv+1, input_count);
    sb_strip(&sb);
    auto input = sb_borrow(&sb);
    Die dice[MAX_DICE];
    DiceExpression de = {.data=dice, .count = 0, .capacity = MAX_DICE};
    auto e = parse_dice_expression(input, &de).errored;
    if(e){
        report_error(e);
        return e;
        }
    RngState rng_ = {};
    auto rng = &rng_;
    seed_rng(rng);
    roll_and_display(de, rng);
    return 0;
    }
