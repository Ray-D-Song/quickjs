#!/bin/bash
# Script to generate complete opcode mapping JSON file

echo "Generating complete opcode mapping..."

cat << 'EOF' > temp_opcode_mapper.c
#include "quickjs.h"
#include "quickjs-opcode.h"
#include <stdio.h>

typedef enum OPCodeEnum {
#define FMT(f)
#define DEF(id, size, n_pop, n_push, f) OP_ ## id,
#define def(id, size, n_pop, n_push, f)
#include "quickjs-opcode.h"
#undef def
#undef DEF
#undef FMT
    OP_COUNT,
} OPCodeEnum;

const char* opcode_names[] = {
#define FMT(f)
#define DEF(id, size, n_pop, n_push, f) #id,
#define def(id, size, n_pop, n_push, f)
#include "quickjs-opcode.h"
#undef def
#undef DEF
#undef FMT
};

int main() {
    printf("{\n");
    for (int i = 0; i < OP_COUNT; i++) {
        printf("  \"%d\": \"%s\"", i, opcode_names[i]);
        if (i < OP_COUNT - 1) {
            printf(",");
        }
        printf("\n");
    }
    printf("}\n");
    return 0;
}
EOF

gcc -I. temp_opcode_mapper.c -o temp_opcode_mapper && ./temp_opcode_mapper > opcodes.json
rm temp_opcode_mapper temp_opcode_mapper.c

echo "Opcode mapping saved to opcodes.json"
echo "Total opcodes: $(grep -c '":' opcodes.json)"