#include <assembler_x64.h>
#include <string.h>

const fpu_register fpu_tmp = XMM0;
const asm_register tmp = R11;

static void
set_mod_rm(operand* self, int mod, asm_register reg){
    if((reg > 7) && !((reg == R12) && (mod != 3))){
        self->rex |= REX_B;
    }
    self->encoding[0] = (uint8_t) ((mod << 6) | (reg & 7));
    self->length = 1;
}

static void
set_sib(operand* self, scale_factor scale, asm_register index, asm_register base){
    if(base & 7){
        self->rex |= REX_B;
    }
    if(index > 7){
        self->rex |= REX_X;
    }
    self->encoding[1] = (scale << 6) | ((index & 7) << 3) | (base & 7);
    self->length = 2;
}

static void
set_disp_8(operand* self, int8_t disp){
    self->encoding[self->length++] = (uint8_t) disp;
}

static void
set_disp_32(operand* self, int32_t disp){
    memmove(&self->encoding[self->length], &disp, sizeof(disp));
    self->length += sizeof(disp);
}

operand*
operand_register_new(asm_register reg){
    operand* op = malloc(sizeof(operand));
    op->rex = REX_NONE;
    set_mod_rm(op, 3, reg);
    return op;
}

operand*
operand_clone_new(operand* other){
    operand* self = malloc(sizeof(operand));
    self->length = other->length;
    self->rex = other->rex;
    memmove(&self->encoding[0], &other->encoding[0], other->length);
    return self;
}

operand*
operand_new(){
    operand* self = malloc(sizeof(operand));
    self->length = 0;
    self->rex = REX_NONE;
    return self;
}

address*
address_r_fixed_new(asm_register base, int32_t disp, bool fixed){
    address* addr = malloc(sizeof(address));
    set_mod_rm(addr, 2, base);
    if((base & 7) == RSP){
        set_sib(addr, TIMES_1, RSP, base);
    }
    set_disp_32(addr, disp);
    return addr;
}

address*
address_r_new(asm_register base, int32_t disp){
    address* addr = malloc(sizeof(address));
    if((disp == 0) && (base & 7) != RBP){
        set_mod_rm(addr, 0, base);
        if((base & 7) == RSP){
            set_sib(addr, TIMES_1, RSP, base);
        }
    } else{
        set_mod_rm(addr, 2, base);
        if((base & 7) == RSP){
            set_sib(addr, TIMES_1, RSP, base);
        }
        set_disp_32(addr, disp);
    }
    return addr;
}

address*
address_r_scale_new(asm_register index, scale_factor scale, int32_t disp){
    address* addr = malloc(sizeof(address));
    set_mod_rm(addr, 0, RSP);
    set_sib(addr, scale, index, RBP);
    set_disp_32(addr, disp);
    return addr;
}

address*
address_rr_scale_new(asm_register base, asm_register index, scale_factor scale, int32_t disp){
    address* addr = malloc(sizeof(address));
    if((disp == 0) && ((base & 7) != RBP)){
        set_mod_rm(addr, 0, RSP);
        set_sib(addr, scale, index, base);
    } else{
        set_mod_rm(addr, 2, RSP);
        set_sib(addr, scale, index, base);
        set_disp_32(addr, disp);
    }
    return addr;
}

address*
address_clone_new(address* other){
    return operand_clone_new(other);
}

field_address*
field_address_new(asm_register base, int32_t disp){
    return address_r_new(base, disp);
}

field_address*
field_address_rr_scale_new(asm_register base, asm_register index, scale_factor scale, int32_t disp){
    return address_rr_scale_new(base, index, scale, disp);
}

field_address*
field_address_clone_new(field_address* other){
    return address_clone_new(other);
}

assembler*
assembler_new(){
    assembler* assm = malloc(sizeof(assembler));
    assm->buffer = asm_buffer_new();
    return assm;
}

void
assembler_emit_operand(assembler* self, int rm, operand* op){
    const word length = op->length;
    assembler_emit_uint8(self, (uint8_t) (op->encoding[0] + (rm << 3)));
    for(word i = 1; i < length; i++){
        assembler_emit_uint8(self, op->encoding[i]);
    }
}

void
assembler_emit_label(assembler* self, label* lbl, word instr_size){
    if(label_is_bound(lbl)){
        word offset = label_position(lbl) - (asm_buffer_size(self->buffer));
        assembler_emit_int32(self, (int32_t) (offset - instr_size));
    } else{
        assembler_emit_label_link(self, lbl);
    }
}

static inline void
label_link_to(label* self, word pos){
    self->pos = pos + sizeof(word);
}

static inline void
label_bind_to(label* self, word pos){
    self->pos = -pos - sizeof(word);
}

void
assembler_emit_label_link(assembler* self, label* lbl){
    word pos = asm_buffer_size(self->buffer);
    assembler_emit_int32(self, (int32_t) lbl->pos);
    label_link_to(lbl, pos);
}

void
assembler_emit_immediate(assembler* self, immediate* imm){
    assembler_emit_int64(self, *imm);
}

void
assembler_ret(assembler* self){
    assembler_emit_uint8(self, 0xC3);
}

void
assembler_bind(assembler* self, label* lbl){
    word bound = asm_buffer_size(self->buffer);
    while(label_is_linked(lbl)){
        word pos = label_link_position(lbl);
        word next = asm_buffer_load_int32(self->buffer, pos);
        asm_buffer_store_int32(self->buffer, pos, (int32_t) (bound - (pos + 4)));
        lbl->pos = next;
    }
    label_bind_to(lbl, bound);
}

void
assembler_addq_ra(assembler* self, asm_register dst, address* addr){
    assembler_emit_operand_rex(self, dst, addr, REX_W);
    assembler_emit_uint8(self, 0x03);
    assembler_emit_operand(self, dst & 7, addr);
}

void
assembler_addq_rr(assembler* self, asm_register dst, asm_register src){
    operand* op = operand_register_new(src);
    assembler_emit_operand_rex(self, dst, op, REX_W);
    assembler_emit_uint8(self, 0x03);
    assembler_emit_operand(self, dst & 7, op);
    free(op);
}

void
assembler_addq_ri(assembler* self, asm_register dst, immediate* imm){
    assembler_movq_ri(self, tmp, imm);
    assembler_addq_rr(self, dst, tmp);
}

void
assembler_addq_ar(assembler* self, address* addr, asm_register src){
    assembler_emit_operand_rex(self, src, addr, REX_W);
    assembler_emit_uint8(self, 0x01);
    assembler_emit_operand(self, src & 7, addr);
}

void
assembler_addq_ai(assembler* self, address* addr, immediate* imm){
    assembler_movq_ri(self, tmp, imm);
    assembler_addq_ar(self, addr, tmp);
}

void
assembler_movq_ri(assembler* self, asm_register dst, immediate* imm){
    assembler_emit_register_rex(self, dst, REX_W);
    assembler_emit_uint8(self, (uint8_t) ((0xB8) | (dst & 7)));
    assembler_emit_immediate(self, imm);
}

void
assembler_jmp(assembler* self, label* lbl, bool near){
    if(label_is_bound(lbl)){
        word offset = label_position(lbl) - asm_buffer_size(self->buffer);
        assembler_emit_uint8(self, 0xE9);
        assembler_emit_int32(self, (int32_t) (offset - 5));
    } else if(near){
        assembler_emit_uint8(self, 0xEB);
    } else{
        assembler_emit_uint8(self, 0xE9);
        assembler_emit_label_link(self, lbl);
    }
}

void*
assembler_compile(assembler* self){
    void* ptr = mmap(0, assembler_size(self), PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANON, -1, 0);
    if(ptr == NULL){
        fprintf(stderr, "Cannot compile code buffer\n");
        abort();
    }
    memcpy(ptr, assembler_code(self), assembler_size(self));
    return ptr;
}