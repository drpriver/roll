#ifndef SLICE_H
#define SLICE_H
#include <string.h>
#include "david_macros.h"
#define Slice(T) Slice__##T
#define Slice_remove_at(T) Slice_remove_at__##T
#define Slice_remove(T) Slice_remove__##T
#define Slice_ordered_remove_at(T) Slice_ordered_remove_at__##T
#define Slice_append(T) Slice_append__##T
#define Slice_append_p(T) Slice_append_p__##T
#define Slice_pop(T) Slice_pop__##T
#define Slice_copy(T) Slice_copy__##T
#define Slice_contains(T) Slice_contains__##T
#define Slice_index_of(T) Slice_index_of__##T
#define Slice_from_array(arr) {.data=arr, .count=0, .capacity=arrlen(arr),}
#define Slice_from_void_slice(T, source_slice) ({\
        auto s_ = source_slice;\
        assert(s_.nbytes % sizeof(T) == 0);\
        assert(source_slice.pointer);\
        (Slice(T)){.capacity = s_.nbytes/sizeof(T), .count=0, .data = (T*)s_.pointer};\
        })

#define SliceDeclare(T) typedef struct Slice(T) {\
    int capacity;\
    int count;\
    Nonnull(T*) data;\
    } Slice(T);
#define SliceDeclareImpl(T) \
PushDiagnostic() \
SuppressUnusedFunction() \
static inline void Slice_remove_at(T)(Nonnull(Slice(T)*) slice, int index){\
    assert(index >= 0);\
    assert(slice->count);\
    assert(index < slice->capacity);\
    slice->data[index] = slice->data[slice->count-1];\
    slice->count--;\
    }\
static inline void Slice_remove(T)(Nonnull(Slice(T)*) slice, T value){\
    for(int i = 0; i < slice->count; i++){\
        if(memcmp(&slice->data[i], &value, sizeof(T))==0){\
            Slice_remove_at(T)(slice, i);\
            return;\
            }\
        }\
    return;\
    }\
static inline void Slice_ordered_remove_at(T)(Nonnull(Slice(T)*)slice, int index){\
    assert(index >= 0);\
    assert(slice->count);\
    assert(index < slice->capacity);\
    if(index == slice->count-1){\
        Slice_remove_at(T)(slice, index);\
        return;\
        }\
    memmove(slice->data + index, slice->data+index+1, sizeof(T)*(slice->count - index - 1));\
    slice->count--;\
    }\
static inline force_inline void Slice_append(T)(Nonnull(Slice(T)*) slice, T value){\
    assert(slice->capacity > slice->count);\
    slice->data[slice->count] = value;\
    slice->count++;\
    }\
static inline void Slice_append_p(T)(Nonnull(Slice(T)*) slice, Nonnull(T*) value){\
    assert(slice->capacity > slice->count);\
    slice->data[slice->count] = *value;\
    slice->count++;\
    }\
static inline T Slice_pop(T)(Nonnull(Slice(T)*) slice){\
    assert(slice->count);\
    T result = slice->data[slice->count-1];\
    slice->count--;\
    return result;\
    }\
static inline void Slice_copy(T)(Nonnull(Slice(T)*) dst, Nonnull(const Slice(T)*) src){\
    assert(dst->capacity >= src->count);\
    if(not src->count)\
        return;\
    assert(src->count);\
    memcpy(dst->data, src->data, sizeof(T)*(src->count));\
    dst->count = src->count;\
    }\
static inline bool Slice_contains(T)(Nonnull(const Slice(T)*) slice, T value){\
    for(int i = 0; i < slice->count; i++){\
        if(memcmp(&slice->data[i], &value, sizeof(T))==0)\
            return true;\
        }\
    return false;\
    }\
static inline int Slice_index_of(T)(Nonnull(const Slice(T)*)slice, T value){\
    for(int i = 0; i < slice->count; i++){\
        if(memcmp(&slice->data[i], &value, sizeof(T))==0)\
            return i;\
        }\
    return -1;\
    }\
PopDiagnostic()

#define Slice_shuffle(T) Slice_shuffle_##T
#define Slice_pop_random(T) Slice_pop_random_##T

#define SliceRandomDeclare(T)\
PushDiagnostic() \
SuppressUnusedFunction() \
static inline T Slice_pop_random(T)(Nonnull(Slice(T)*) slice, Nonnull(RngState*) rng){\
    int index = bounded_random(rng, slice->count);\
    auto v = slice->data[index];\
    Slice_remove_at(T)(slice, index);\
    return v;\
    }\
PopDiagnostic()
SliceDeclare(uint32_t);
SliceDeclare(int32_t);
SliceDeclare(uint64_t);
SliceDeclare(size_t);
SliceDeclare(int64_t);
SliceDeclare(uintptr_t);
SliceDeclareImpl(uint32_t);
SliceDeclareImpl(int32_t);
SliceDeclareImpl(uint64_t);
SliceDeclareImpl(size_t);
SliceDeclareImpl(int64_t);
SliceDeclareImpl(uintptr_t);

#endif
