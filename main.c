#include <assembler.h>

typedef struct{
    int x;
    int y;
} point;

typedef void (*func)();

int main(int argc, char** argv){
    assembler* assm = assembler_new();
    point* p = malloc(sizeof(point));
    p->x = 100;
    p->y = 100;

    field_address* yAddr = field_address_new(RAX, offsetof(point, y));
    immediate ten = 1000;
    immediate pointImm = (immediate) p;

    assembler_movq_ri(assm, RAX, &pointImm);
    assembler_addq_ai(assm, yAddr, &ten);
    assembler_ret(assm);

    ((func) assembler_compile(assm))();
    printf("%d\n", p->y);
    return 0;
}