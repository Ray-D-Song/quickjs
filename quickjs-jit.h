#ifndef QUICKJS_JIT_H
#define QUICKJS_JIT_H

#ifdef QJS_JIT_ENABLED

#include "quickjs.h"
#include "sljitLir.h"

#define JIT_HOTNESS_THRESHOLD 10

typedef struct JSFunctionBytecode JSFunctionBytecode;

typedef struct JSJITFunction {
  void *native_code;
  size_t code_size;
  int hotness_counter;
  bool is_compiled;
  struct sljit_compiler *compiler;
  JSFunctionBytecode *bytecode;
} JSJITFunction;

// lifecycle
void js_jit_init(JSContext *ctx, JSFunctionBytecode *b);
void js_jit_cleanup(JSRuntime *rt, JSJITFunction *jit_func);

// compile
bool js_jit_compile(JSContext *ctx, JSJITFunction *jit_func);

// execute
void js_jit_execute(JSContext *ctx, JSJITFunction *jit_func, JSValueConst this_obj, JSValueConst *argv);

// hotness
bool js_jit_should_compile(JSFunctionBytecode *b);
void js_jit_increment_hotness(JSFunctionBytecode *b);

// accessor functions for JSFunctionBytecode
JSJITFunction* js_function_get_jit(JSFunctionBytecode *b);
void js_function_set_jit(JSFunctionBytecode *b, JSJITFunction *jit);


#endif /* QJS_JIT_ENABLED */
#endif /* QUICKJS_JIT_H */