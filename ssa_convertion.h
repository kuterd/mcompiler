#ifndef SSA_CONVERTION
#include "ir.h"


// Convert the function to the ssa form.
void ssa_convert(struct ir_context *ctx, struct function *fun); 

// Convert back from the ssa form.
void ssa_convertBack(struct ir_context *ctx, struct function *fun);



#endif
