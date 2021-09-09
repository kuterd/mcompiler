#ifndef SSA_CONVERTION
#include "ir.h"
#include "dominators.h"

// Convert the function to the ssa form.
void ssa_convert(ir_context_t *ctx, function_t *fun,
    struct dominators *doms, struct domfrontiers *df);

// Convert back from the ssa form.
// This will insert new copies as needed.
void ssa_convertBack(ir_context_t *ctx, function_t *fun);

#endif
