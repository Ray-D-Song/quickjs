#ifdef QJS_JIT_ENABLED
#include "quickjs-jit.h"
#include "quickjs.h"

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

  js_free(rt, jit_func);
}

bool js_jit_compile(JSContext *ctx, JSJITFunction *jit_func)
{
  jit_func->compiler = sljit_create_compiler(NULL);
  if (!jit_func->compiler)
  {
    return false;
  }

  sljit_emit_enter(jit_func->compiler, 0, SLJIT_ARGS3(P, P, W), 3, 3, 0, 0, 0);
  sljit_emit_return(jit_func->compiler, SLJIT_MOV, SLJIT_IMM, 42);

  jit_func->native_code = sljit_generate_code(jit_func->compiler);
  if (!jit_func->native_code)
  {
    return false;
  }

  jit_func->is_compiled = true;
  return true;
}
#endif /* QJS_JIT_ENABLED */