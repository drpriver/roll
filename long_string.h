#ifndef LONG_STRING_H
#define LONG_STRING_H
#include <stdlib.h>
#include <string.h>
#include "david_macros.h"

typedef struct LongString {
    size_t length; // excludes the terminating NUL
    NullUnspec(char*) text;
} LongString;



#endif
