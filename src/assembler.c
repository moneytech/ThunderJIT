#include <assembler.h>
#include <stdlib.h>

static const word kMinGap = 32;
static const word kInitialBufferCapacity = 4 * (1024 * 1024);

assembler_buffer*
asm_buffer_new(){
    assembler_buffer* buffer = malloc(sizeof(assembler_buffer));
    buffer->contents = (uword) malloc(kInitialBufferCapacity);
    buffer->cursor = buffer->contents;
    buffer->limit = buffer->contents + kInitialBufferCapacity + kMinGap;
    return buffer;
}