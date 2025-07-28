#!/bin/bash
# Script to find opcode names quickly

opcode=$1
if [ -z "$opcode" ]; then
    echo "Usage: $0 <opcode_number>"
    exit 1
fi

cat << EOF > temp_opcode_finder.c
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
    printf("Opcode $opcode is: %s\n", opcode_names[$opcode]);
    return 0;
}
EOF

gcc -I. temp_opcode_finder.c -o temp_opcode_finder && ./temp_opcode_finder
rm temp_opcode_finder temp_opcode_finder.c