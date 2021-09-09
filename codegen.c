#include "codegen.h"
#include "list.h"
#include "x86_64_codegen.h"

void _lruBump(struct codegen *cg, struct variable *var);

void codegen_init(struct codegen *cg, size_t registerCount) {
    *cg = (struct codegen){};

    dbuffer_init(&cg->buffer);
    cg->registerCount = registerCount;

    LIST_INIT(&cg->lruVariables);
    cg->registerStatus = dzmalloc(sizeof(void *) * cg->registerCount);
}

void codegen_pushBlock(struct codegen *cg, label_t *label) {
    label_setOffset(label, cg->buffer.usage);
}

void codegen_popBlock(struct codegen *cg) {
    // spill dirty registers.
    for (int i = 0; i < cg->registerCount; i++) {
        struct variable *var = cg->registerStatus[i];
        if (var == NULL)
            continue;
        variable_store(cg, cg->registerStatus[i]);
    }
}

struct variable *codegen_newVarReg(struct codegen *cg, int reg) {
    struct variable *var = nnew(struct variable);
    var->reg = reg;
    var->stackPos = -1; // no stack pos.
    cg->registerStatus[reg] = var;

    list_add(&cg->lruVariables, &var->list);
    return var;
}

struct variable *codegen_newVar(struct codegen *cg) {
    int reg = codegen_allocateReg(cg);
    return codegen_newVarReg(cg, reg);
}

struct variable *codegen_newTmp(struct codegen *cg) {
    struct variable *var = nnew(struct variable);
    var->isTmp = 1;
    var->reg = -1;      // no reg.
    var->stackPos = -1; // no stack pos.
}

void codegen_initFunction(struct codegen *cg, int argCount,
                          struct variable **vars) {
    // TODO: Set arguments here !!
    // setup variables here.
    for (int i = 0; i < argCount; i++)
        vars[i] = arch_initFunctionArg(cg, i);
}

// Backup the variable but keep it's registers.
/*
void variable_backup(struct codegen *cg, struct variable *var) {
    if (var->stackPos < 0)
        var->stackPos = ++cg->frameSize;
     arch_spill(cg, var);
}
*/

void variable_store(struct codegen *cg, struct variable *var) {
    // Allocate a new slot.
    if (var->stackPos < 0)
        var->stackPos = ++cg->frameSize;

    // Remove from the lru list.
    arch_spill(cg, var);
    list_deattach(&var->list);
    cg->registerStatus[var->reg] = NULL;
    var->reg = -1;
}

int codegen_allocateReg(struct codegen *cg) {
    for (int i = 0; i < cg->registerCount; i++) {
        // Found a free register.
        if (cg->registerStatus[i] == NULL)
            return i;
    }
    // Free a register.
    assert(cg->lruVariables.next != NULL && "invalid state");
    struct variable *var =
        containerof(cg->lruVariables.next, struct variable, list);
    int reg = var->reg;

    variable_store(cg, var);
    return reg;
}

void _lruBump(struct codegen *cg, struct variable *var) {
    list_deattach(&var->list);
    list_add(&cg->lruVariables, &var->list);
}

// Get or allocate a register for a variable.
// The register allocator might free the register that this variable lives on
// at any time. This needs to be called every time variable is used.
int variable_ref(struct codegen *cg, struct variable *var) {
    if (var->reg >= 0) {
        _lruBump(cg, var);
        return var->reg;
    }
    int result = var->reg = codegen_allocateReg(cg);
    cg->registerStatus[result] = var;
    arch_loadReg(cg, var);
    list_add(&cg->lruVariables, &var->list);

    return result;
}
