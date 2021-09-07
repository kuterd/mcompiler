#ifndef SSA_CONVERTION
#include "ir.h"
#include "dominators.h"

// Convert the function to the ssa form.
void ssa_convert(struct ir_context *ctx, function_t *fun,
    struct dominators *doms, struct domfrontiers *df);

// Convert back from the ssa form.
void ssa_convertBack(struct ir_context *ctx, function_t *fun);

#endif
