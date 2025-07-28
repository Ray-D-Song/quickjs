#ifdef QJS_JIT_ENABLED
#include "quickjs.h"
#include "quickjs-jit.h"
#include "quickjs-opcode.h"
#include "sljitLir.h"

/* Opcode enum generation - must match quickjs.c */
typedef enum OPCodeEnum
{
#define FMT(f)
#define DEF(id, size, n_pop, n_push, f) OP_##id,
#define def(id, size, n_pop, n_push, f)
#include "quickjs-opcode.h"
#undef def
#undef DEF
#undef FMT
  OP_COUNT,
} OPCodeEnum;

void js_jit_init(JSContext *ctx, JSFunctionBytecode *b)
{
  if (js_function_get_jit(b) != NULL)
  {
    return;
  }
  JSJITFunction *jit_func = js_malloc(ctx, sizeof(JSJITFunction));
  if (!jit_func)
  {
    return;
  }
  jit_func->native_code = NULL;
  jit_func->code_size = 0;
  jit_func->hotness_counter = 0;
  jit_func->is_compiled = false;
  jit_func->compiler = NULL;
  jit_func->bytecode = b;
  js_function_set_jit(b, jit_func);
}

void js_jit_cleanup(JSRuntime *rt, JSJITFunction *jit_func)
{
  if (!jit_func)
    return;

  if (jit_func->native_code)
  {
    sljit_free_code(jit_func->native_code, NULL);
  }

  if (jit_func->compiler)
  {
    sljit_free_compiler(jit_func->compiler);
  }

  js_free_rt(rt, jit_func);
}

/* Helper macros for stack operations */
#define STACK_PUSH(compiler, stack_top_reg, value_src, value_offset) \
  do                                                                 \
  {                                                                  \
    sljit_emit_op1(compiler, SLJIT_MOV,                              \
                   SLJIT_MEM2(SLJIT_SP, stack_top_reg), 3,           \
                   value_src, value_offset);                         \
    sljit_emit_op2(compiler, SLJIT_ADD, stack_top_reg, 0,            \
                   stack_top_reg, 0, SLJIT_IMM, 8);                  \
  } while (0)

#define STACK_POP(compiler, stack_top_reg, dest_reg)        \
  do                                                        \
  {                                                         \
    sljit_emit_op2(compiler, SLJIT_SUB, stack_top_reg, 0,   \
                   stack_top_reg, 0, SLJIT_IMM, 8);         \
    sljit_emit_op1(compiler, SLJIT_MOV, dest_reg, 0,        \
                   SLJIT_MEM2(SLJIT_SP, stack_top_reg), 3); \
  } while (0)

#define STACK_TOP(compiler, stack_top_reg, dest_reg) \
  sljit_emit_op1(compiler, SLJIT_MOV, dest_reg, 0,   \
                 SLJIT_MEM2(SLJIT_SP, stack_top_reg), 3)

bool js_jit_compile(JSContext *ctx, JSJITFunction *jit_func)
{
  jit_func->compiler = sljit_create_compiler(NULL);
  if (!jit_func->compiler)
  {
    return false;
  }

  uint8_t *bc_ptr = js_function_get_bytecode_ptr(jit_func->bytecode);
  uint32_t bc_len = js_function_get_bytecode_len(jit_func->bytecode);

  /* Calculate required stack space: stack_size * 8 bytes per slot */
  sljit_sw local_size = js_function_get_stack_size(jit_func->bytecode) * 8;

  /* Enter function with proper register allocation and local storage
   * Return: sljit_sw (W)
   * Args: JSContext *ctx (P), JSValue *sp (P), int argc (W)
   * Registers: 3 scratch (R0-R2), 2 saved (S0-S1), local_size bytes
   * S0 = virtual stack top offset (in bytes)
   * S1 = JSContext *ctx (saved for function calls)
   */
  sljit_emit_enter(jit_func->compiler, 0, SLJIT_ARGS3(W, P, P, W),
                   3, 2, local_size);

  /* Initialize virtual stack
   * S0 = stack top offset (starts at 0)
   * S1 = JSContext *ctx (first parameter)
   */
  sljit_emit_op1(jit_func->compiler, SLJIT_MOV, SLJIT_S0, 0, SLJIT_IMM, 0);
  sljit_emit_op1(jit_func->compiler, SLJIT_MOV, SLJIT_S1, 0, SLJIT_S(0), 0);

  uint8_t *bc_end = bc_ptr + bc_len;
  while (bc_ptr < bc_end)
  {
    uint8_t opcode = *bc_ptr++;
    switch (opcode)
    {
    case OP_push_i32:
    {
      int32_t val = *(int32_t *)bc_ptr;
      bc_ptr += 4;
      STACK_PUSH(jit_func->compiler, SLJIT_S0, SLJIT_IMM, val);
      break;
    }

    case OP_push_0:
      STACK_PUSH(jit_func->compiler, SLJIT_S0, SLJIT_IMM, 0);
      break;

    case OP_push_1:
      STACK_PUSH(jit_func->compiler, SLJIT_S0, SLJIT_IMM, 1);
      break;

    case OP_add:
    {
      /* Pop two values, add them, push result */
      STACK_POP(jit_func->compiler, SLJIT_S0, SLJIT_R1); /* second operand */
      STACK_POP(jit_func->compiler, SLJIT_S0, SLJIT_R0); /* first operand */
      sljit_emit_op2(jit_func->compiler, SLJIT_ADD,
                     SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
      STACK_PUSH(jit_func->compiler, SLJIT_S0, SLJIT_R0, 0);
      break;
    }

    case OP_sub:
    {
      /* Pop two values, subtract them, push result */
      STACK_POP(jit_func->compiler, SLJIT_S0, SLJIT_R1); /* second operand */
      STACK_POP(jit_func->compiler, SLJIT_S0, SLJIT_R0); /* first operand */
      sljit_emit_op2(jit_func->compiler, SLJIT_SUB,
                     SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
      STACK_PUSH(jit_func->compiler, SLJIT_S0, SLJIT_R0, 0);
      break;
    }

    case OP_mul:
    {
      /* Pop two values, multiply them, push result */
      STACK_POP(jit_func->compiler, SLJIT_S0, SLJIT_R1); /* second operand */
      STACK_POP(jit_func->compiler, SLJIT_S0, SLJIT_R0); /* first operand */
      sljit_emit_op2(jit_func->compiler, SLJIT_MUL,
                     SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
      STACK_PUSH(jit_func->compiler, SLJIT_S0, SLJIT_R0, 0);
      break;
    }

    case OP_return:
    {
      /* Check if there's a value on the stack to return */
      struct sljit_jump *empty_stack = sljit_emit_cmp(jit_func->compiler,
                                                      SLJIT_EQUAL, SLJIT_S0, 0, SLJIT_IMM, 0);

      /* Stack not empty: pop and return the value */
      STACK_POP(jit_func->compiler, SLJIT_S0, SLJIT_RETURN_REG);
      struct sljit_jump *return_jump = sljit_emit_jump(jit_func->compiler, SLJIT_JUMP);

      /* Stack empty: return 0 */
      sljit_set_label(empty_stack, sljit_emit_label(jit_func->compiler));
      sljit_emit_op1(jit_func->compiler, SLJIT_MOV,
                     SLJIT_RETURN_REG, 0, SLJIT_IMM, 0);

      /* Common return point */
      sljit_set_label(return_jump, sljit_emit_label(jit_func->compiler));
      sljit_emit_return(jit_func->compiler, SLJIT_MOV, SLJIT_RETURN_REG, 0);
      break;
    }

    case OP_nop:
      break;

    default:
      sljit_free_compiler(jit_func->compiler);
      jit_func->compiler = NULL;
      return false;
    }
  }

  /* If we reach here without explicit return, return 0 */
  sljit_emit_op1(jit_func->compiler, SLJIT_MOV, SLJIT_RETURN_REG, 0, SLJIT_IMM, 0);
  sljit_emit_return(jit_func->compiler, SLJIT_MOV, SLJIT_RETURN_REG, 0);

  jit_func->native_code = sljit_generate_code(jit_func->compiler, 0, NULL);
  if (!jit_func->native_code)
  {
    return false;
  }

  jit_func->is_compiled = true;
  return true;
}
#endif /* QJS_JIT_ENABLED */