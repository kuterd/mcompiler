#ifndef CODEGEN_H
#define CODEGEN_H

#include "buffer.h"
#include "list.h"
#include "relocation.h"
#include "utils.h"
#include "x86_64.h"

/*
struct codegen_arch {
    void (*emit_blockEntry)(struct codegen *ch, struct );
    void (*emit_blockExit)(struct codegen *ch);
};
*/

struct codegen {
    // General purpose registers.
    int registerCount;
    // Output.
    dbuffer_t buffer;

    // -- Function generation stuff --
    // size of the stack frame, slot size (8 bytes for x86_64)
    size_t frameSize;

    // -- Register allocation stuff --
    // Variables that are stored on registers.
    struct list_head lruVariables;
    // Sometimes we need to free a specific register.
    // for example when we need to do a function call.
    struct variable **registerStatus; // register -> variable;
};

// This can be stored on a hashmap.
struct variable {
    // TODO track if dirty state.
    // Tmp variable
    int isTmp;
    int reg;               // register id.
    int stackPos;          // stack position if there is one.
    struct list_head list; // LRU cache for variables stored on registers.
};

void codegen_init(struct codegen *cg, size_t registerCount);
void codegen_pushBlock(struct codegen *cg, label_t *label);
void codegen_popBlock(struct codegen *cg);

// Usefull for things like function arguments.
struct variable *codegen_newVarReg(struct codegen *cg, int reg);
struct variable *codegen_newVar(struct codegen *cg);

void codegen_initFunction(struct codegen *cg, int argCount,
                          struct variable **vars);

void variable_freeReg(struct codegen *cg, struct variable *var);

int codegen_allocateReg(struct codegen *cg);

// Get or allocate a register for a variable.
// The register allocator might free the register that this variable lives on
// at any time. This needs to be called every time variable is used.
int variable_ref(struct codegen *cg, struct variable *var);
void variable_store(struct codegen *cg, struct variable *var);

#endif
