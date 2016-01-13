#ifndef THUNDERJIT_ASSEMBLER_H
#define THUNDERJIT_ASSEMBLER_H

#include <stdint.h>
#include <stddef.h>

typedef uintptr_t uword;
typedef intptr_t word;

typedef struct{
    uword contents;
    uword cursor;
    uword limit;
} assembler_buffer;

assembler_buffer* asm_buffer_new();

static inline void
asm_buffer_emit_uint8(assembler_buffer* self, uint8_t value){
    *((uint8_t*) self->cursor) = value;
    self->cursor += sizeof(uint8_t);
}

static inline void
asm_buffer_emit_int32(assembler_buffer* self, int32_t val){
    *((int32_t*) self->cursor) = val;
    self->cursor += sizeof(int32_t);
}

static inline void
asm_buffer_emit_int64(assembler_buffer* self, int64_t val){
    *((int64_t*) self->cursor) = val;
    self->cursor += sizeof(int64_t);
}

static inline word
asm_buffer_size(assembler_buffer* self){
    return self->cursor - self->contents;
}

static inline int32_t
asm_buffer_load_int32(assembler_buffer* self, word pos){
    return (int32_t) (self->contents + pos);
}

static inline void
asm_buffer_store_int32(assembler_buffer* self, word pos, int32_t val){
    *((int32_t*) (self->contents + pos)) = val;
}

#include "assembler_x64.h"

#endif