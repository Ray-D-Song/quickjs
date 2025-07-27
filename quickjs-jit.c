#ifdef QJS_JIT_ENABLED
#include "quickjs.h"
#include "quickjs-jit.h"
#include "quickjs-opcode.h"

#define JS_GET_BYTECODE_PTR(b) ((uint8_t *)((uint8_t *)(b) + sizeof(JSFunctionBytecode)))

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

bool js_jit_compile(JSContext *ctx, JSJITFunction *jit_func)
{
  jit_func->compiler = sljit_create_compiler(NULL);
  if (!jit_func->compiler)
  {
    return false;
  }

  sljit_emit_enter(jit_func->compiler, 0, SLJIT_ARGS3(P, P, W), 3, 3, 0, 0);

  uint8_t *bc_ptr = JS_GET_BYTECODE_PTR(jit_func->bytecode);
  uint32_t bc_len = jit_func->bytecode->byte_code_len;

  /* Use memory as virtual registers */
  sljit_s32 stack_base = SLJIT_MEM1(SLJIT_SP);
  sljit_s32 stack_top = 0;

  while (bc_ptr < JS_GET_BYTECODE_PTR(jit_func->bytecode) + bc_len)
  {
    uint8_t opcode = *bc_ptr++;
    switch (opcode) {
      case OP_push_i32: {
        int32_t val = *(int32_t *)bc_ptr;
        bc_ptr += 4;
        sljit_emit_op1(jit_func->compiler, SLJIT_MOV, 
                       SLJIT_MEM1(SLJIT_SP), stack_top * 8, 
                       SLJIT_IMM, val);
        stack_top++;
        break;
      }
      case OP_push_0:
        sljit_emit_op1(jit_func->compiler, SLJIT_MOV, 
                       SLJIT_MEM1(SLJIT_SP), stack_top * 8, 
                       SLJIT_IMM, 0);
        stack_top++;
        break;
      case OP_push_1:
        sljit_emit_op1(jit_func->compiler, SLJIT_MOV, 
                       SLJIT_MEM1(SLJIT_SP), stack_top * 8, 
                       SLJIT_IMM, 1);
        stack_top++;
        break;
      case OP_add: {
        sljit_emit_op2(jit_func->compiler, SLJIT_ADD, 
                       SLJIT_MEM1(SLJIT_SP), (stack_top - 2) * 8,
                       SLJIT_MEM1(SLJIT_SP), (stack_top - 2) * 8,
                       SLJIT_MEM1(SLJIT_SP), (stack_top - 1) * 8);
        stack_top--;
        break;
      }
      case OP_sub: {
        sljit_emit_op2(jit_func->compiler, SLJIT_SUB, 
                       SLJIT_MEM1(SLJIT_SP), (stack_top - 2) * 8,
                       SLJIT_MEM1(SLJIT_SP), (stack_top - 2) * 8,
                       SLJIT_MEM1(SLJIT_SP), (stack_top - 1) * 8);
        stack_top--;
        break;
      }
      case OP_mul: {
        sljit_emit_op2(jit_func->compiler, SLJIT_MUL, 
                       SLJIT_MEM1(SLJIT_SP), (stack_top - 2) * 8,
                       SLJIT_MEM1(SLJIT_SP), (stack_top - 2) * 8,
                       SLJIT_MEM1(SLJIT_SP), (stack_top - 1) * 8);
        stack_top--;
        break;
      }
      case OP_return: {
        if (stack_top > 0) {
          sljit_emit_op1(jit_func->compiler, SLJIT_MOV, 
                         SLJIT_RETURN_REG, 0,
                         SLJIT_MEM1(SLJIT_SP), (stack_top - 1) * 8);
        } else {
          sljit_emit_op1(jit_func->compiler, SLJIT_MOV, 
                         SLJIT_RETURN_REG, 0,
                         SLJIT_IMM, 0);
        }
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
  

  jit_func->native_code = sljit_generate_code(jit_func->compiler, 0, NULL);
  if (!jit_func->native_code)
  {
    return false;
  }

  jit_func->is_compiled = true;
  return true;
}
#endif /* QJS_JIT_ENABLED */