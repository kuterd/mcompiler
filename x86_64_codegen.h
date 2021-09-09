#ifndef X86_64_CODEGEN
#define X86_64_CODEGEN

// TODO: Target Abstraction.

int arch_getRealReg(struct variable *var);
struct variable *arch_initFunctionArg(struct codegen *cg, int i);
void arch_loadReg(struct codegen *cg, struct variable *var);
void arch_functionCallArg(struct codegen *cg, struct variable *var, int i);
void arch_functionCall(struct codegen *cg, label_t *label, size_t count,
                       struct variable **var);
void arch_spill(struct codegen *cg, struct variable *var);

#endif
