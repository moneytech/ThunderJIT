#ifndef THUNDERJIT_ASSEMBLER_X64_H
#define THUNDERJIT_ASSEMBLER_X64_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "assembler.h"
#include "constants_x64.h"
#include <sys/mman.h>

typedef struct{
    word pos;
    word unresolved;
} label;

static inline word
label_position(label* self){
    return -self->pos - sizeof(word);
}

static inline word
label_link_position(label* self){
    return self->pos - sizeof(word);
}

static inline bool
label_is_bound(label* self){
    return self->pos < 0;
}

static inline bool
label_is_linked(label* self){
    return self->pos > 0;
}

typedef int64_t immediate;

typedef struct{
    uint8_t length;
    uint8_t rex;
    uint8_t encoding[6];
} operand;

static inline uint8_t
operand_encoding_at(operand* self, word index){
    return self->encoding[index];
}

static inline bool
operand_is_register(operand* self, asm_register reg){
    return ((reg > 7 ? 1 : 0) == (self->rex & REX_B)) &&
           ((operand_encoding_at(self, 0) & 0xF8) == 0xC0) &&
           ((operand_encoding_at(self, 0) & 0x07) == reg);
}

static inline uint8_t
operand_mod(operand* self){
    return (uint8_t) ((operand_encoding_at(self, 0) >> 6) & 3);
}

static inline asm_register
operand_rm(operand* self){
    int rm_rex = (self->rex & REX_B) << 3;
    return (asm_register) (rm_rex + (operand_encoding_at(self, 0) & 7));
}

static inline asm_register
operand_index(operand* self){
    int index_rex = (self->rex & REX_X) << 2;
    return (asm_register) (index_rex + ((operand_encoding_at(self, 1) >> 3) & 7));
}

static inline asm_register
operand_base(operand* self){
    int base_rex = (self->rex & REX_B) << 3;
    return (asm_register) (base_rex + (operand_encoding_at(self, 1) & 7));
}

operand* operand_register_new(asm_register reg);
operand* operand_clone_new(operand* other);
operand* operand_new();

typedef operand address;

address* address_r_fixed_new(asm_register base, int32_t disp, bool fixed);
address* address_r_new(asm_register base, int32_t disp);
address* address_r_scale_new(asm_register index, scale_factor scale, int32_t disp);
address* address_rr_scale_new(asm_register base, asm_register index, scale_factor scale, int32_t disp);
address* address_clone_new(address* other);

typedef address field_address;

field_address* field_address_new(asm_register base, int32_t disp);
field_address* field_address_rr_scale_new(asm_register base, asm_register index, scale_factor scale, int32_t disp);
field_address* field_address_clone_new(field_address* address);

typedef struct{
    assembler_buffer* buffer;
} assembler;

assembler* assembler_new();

void assembler_emit_immediate(assembler* self, immediate* imm);
void assembler_emit_operand(assembler* self, int rm, operand* op);
void assembler_emit_label(assembler* self, label* lbl, word instr_size);
void assembler_emit_label_link(assembler* self, label* lbl);

static inline void
assembler_emit_uint8(assembler* assm, uint8_t val){
    asm_buffer_emit_uint8(assm->buffer, val);
}

static inline void
assembler_emit_operand_rex(assembler* assm, int rm, operand* op, uint8_t rex){
    rex |= (rm > 7 ? REX_R : REX_NONE) | op->rex;
    if(rex != REX_NONE){
        assembler_emit_uint8(assm, REX_PREFIX|rex);
    }
}

static inline void
assembler_emit_register_rex(assembler* assm, asm_register reg, uint8_t rex){
    rex |= (reg > 7 ? REX_B : REX_NONE);
    if(rex != REX_NONE){
        assembler_emit_uint8(assm, REX_PREFIX|rex);
    }
}

static inline void
assembler_emit_int32(assembler* assm, int32_t val){
    asm_buffer_emit_int32(assm->buffer, val);
}

static inline void
assembler_emit_int64(assembler* assm, int64_t val){
    asm_buffer_emit_int64(assm->buffer, val);
}

static inline uword
assembler_size(assembler* self){
    return self->buffer->cursor - self->buffer->contents;
}

static inline void*
assembler_code(assembler* self){
    return (void*) self->buffer->contents;
}

void* assembler_compile(assembler* self);
void assembler_ret(assembler* self);
void assembler_bind(assembler* self, label* lbl);
void assembler_addq_rr(assembler* self, asm_register dst, asm_register src);
void assembler_addq_ri(assembler* self, asm_register dst, immediate* imm);
void assembler_addq_ra(assembler* self, asm_register dst, address* addr);
void assembler_addq_ar(assembler* self, address* dst, asm_register src);
void assembler_addq_ai(assembler* self, address* dst, immediate* imm);
void assembler_movq_ri(assembler* self, asm_register dst, immediate* imm);
void assembler_jmp(assembler* self, label* lbl, bool near);

#endif