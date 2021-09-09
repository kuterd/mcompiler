#include "buffer.h"
#include "codegen.h"
#include "platform_utils.h"
#include "relocation.h"
#include "x86_64.h"
#include <stdint.h>
#include <stdio.h>

uint8_t kReg8Number[] = {REGISTER8(SECOND3, COMMA)};

uint8_t kReg16Number[] = {REGISTER16(SECOND3, COMMA)};

uint8_t kReg32Number[] = {REGISTER32(SECOND3, COMMA)};

uint8_t kReg64Number[] = {REGISTER64(SECOND3, COMMA)};

// @W Make the addressing 64bit.
// @R Extension for the ModR/M reg field.
// @X Extension of the SIB index field.
// @B Extension for the R/M field or opcode reg field.
void emit_rex(dbuffer_t *dbuffer, uint8_t w, uint8_t r, uint8_t x, uint8_t b) {
    uint8_t result = (0b0100 << 4) | (w << 3) | (r << 2) | (x << 1) | b;
    dbuffer_pushChar(dbuffer, result);
}

// mod
// 0 [rm]
// 1 [rm] + disp8
// 2 [rm] + disp16
// 3 rm
void emit_modrm(dbuffer_t *dbuffer, uint8_t mod, uint8_t regop, uint8_t rm) {
    uint8_t result = (mod << 6) | (regop << 3) | rm;
    dbuffer_pushChar(dbuffer, result);
}

void emit_syscall(dbuffer_t *dbuffer) { dbuffer_push(dbuffer, 2, 0x0F, 0x05); }

void emit_jumpRel8(dbuffer_t *dbuffer, label_t *label) {
    dbuffer_push(dbuffer, 1, 0xEB);
    relocation_emit(dbuffer, label, RELATIVE, INT8, 1);
}

void emit_jumpZeroRel8(dbuffer_t *dbuffer, label_t *label) {
    dbuffer_push(dbuffer, 1, 0x75);
    relocation_emit(dbuffer, label, RELATIVE, INT8, 1);
}

void emit_checkZero32(dbuffer_t *dbuffer, reg32 reg) {
    uint8_t regNumber = kReg32Number[reg];
    if (regNumber > 7) {
        emit_rex(dbuffer, 0, 1, 0, 1);
        regNumber &= 0b111;
    }

    dbuffer_push(dbuffer, 1, 0x85); // test
    emit_modrm(dbuffer, 3, regNumber, regNumber);
}

void emit_checkZero64(dbuffer_t *dbuffer, reg64 reg) {
    uint8_t regNumber = kReg32Number[reg];
    emit_rex(dbuffer, 1, regNumber > 7, 0, regNumber > 7);
    regNumber &= 0b111;

    dbuffer_push(dbuffer, 1, 0x85); // test
    emit_modrm(dbuffer, 3, regNumber, regNumber);
}

// rbp relative load
void emit_loadRegRBP32(dbuffer_t *dbuffer, reg32 reg, char disp) {
    uint8_t regNumber = kReg32Number[reg];
    if (regNumber > 7) {
        emit_rex(dbuffer, 0, 1, 0, 0);
        regNumber &= 0b111;
    }

    dbuffer_push(dbuffer, 1, 0x8B);
    emit_modrm(dbuffer, 1, regNumber, kReg64Number[RBP]);
    dbuffer_push(dbuffer, 1, disp);
}

void emit_loadRegRBP64(dbuffer_t *dbuffer, reg64 reg, char disp) {
    uint8_t regNumber = kReg64Number[reg];
    emit_rex(dbuffer, 1, 0, 0, regNumber > 7);
    regNumber &= 0b111;

    dbuffer_push(dbuffer, 1, 0x8B);
    emit_modrm(dbuffer, 1, regNumber, kReg64Number[RBP]);
    dbuffer_push(dbuffer, 1, disp);
}

void emit_storeConst32(dbuffer_t *dbuffer, reg32 reg, int cons) {
    uint8_t regNumber = kReg64Number[reg];
    if (regNumber > 7) {
        emit_rex(dbuffer, 0, 0, 0, 1);
        regNumber &= 0b111;
    }

    dbuffer_push(dbuffer, 1, 0xB8 | regNumber);
    dbuffer_pushInt(dbuffer, cons, 4);
}

void emit_storeConst64(dbuffer_t *dbuffer, reg64 reg, long cons) {
    uint8_t regNumber = kReg64Number[reg];
    emit_rex(dbuffer, 1, 0, 0, regNumber > 7);
    regNumber &= 0b111;

    dbuffer_push(dbuffer, 1, 0xB8 | regNumber);
    dbuffer_pushLong(dbuffer, cons, 8);
}

void emit_storeLabel64(dbuffer_t *dbuffer, reg64 reg, label_t *label) {
    uint8_t regNumber = kReg64Number[reg];
    emit_rex(dbuffer, 1, 0, 0, regNumber > 7);
    regNumber &= 0b111;

    dbuffer_push(dbuffer, 1, 0xB8 | regNumber);
    relocation_emit(dbuffer, label, ABSOLUTE, INT64, 0);
}

void emit_storeRegRBP64(dbuffer_t *dbuffer, reg64 reg, char disp) {
    uint8_t regNumber = kReg64Number[reg];
    emit_rex(dbuffer, 1, regNumber > 7, 0, 0);
    regNumber &= 0b111;

    dbuffer_push(dbuffer, 1, 0x89); // mov
    emit_modrm(dbuffer, 1, regNumber, kReg64Number[RBP]);
    dbuffer_push(dbuffer, 1, disp);
}

void emit_storeReg64(dbuffer_t *dbuffer, reg64 from, reg64 to) {
    uint8_t fromNumber = kReg64Number[from];
    uint8_t toNumber = kReg64Number[to];
    emit_rex(dbuffer, 1, fromNumber > 7, 0, toNumber > 7);
    fromNumber &= 0b111;
    toNumber &= 0b111;

    dbuffer_push(dbuffer, 1, 0x89); // mov
    emit_modrm(dbuffer, 3, fromNumber, toNumber);
}

void emit_subConst64(dbuffer_t *dbuffer, reg64 reg, int imm) {
    uint8_t regNumber = kReg64Number[reg];
    emit_rex(dbuffer, 1, 0, regNumber > 7, 0);
    regNumber &= 0b111;

    dbuffer_push(dbuffer, 1, 0x81);
    emit_modrm(dbuffer, 3, 5, regNumber);
    dbuffer_pushInt(dbuffer, (unsigned int)imm, 4);
}

void emit_subLabel64(dbuffer_t *dbuffer, reg64 reg, label_t *label) {
    uint8_t regNumber = kReg64Number[reg];
    emit_rex(dbuffer, 1, 0, regNumber > 7, 0);
    regNumber &= 0b111;

    dbuffer_push(dbuffer, 1, 0x81);
    emit_modrm(dbuffer, 3, 5, regNumber);
    relocation_emit(dbuffer, label, ABSOLUTE, INT32, 0);
    // dbuffer_pushInt(dbuffer, (unsigned int)imm, 4);
}

void emit_pushReg(dbuffer_t *dbuffer, reg64 reg) {
    uint8_t regNumber = kReg64Number[reg];
    if (regNumber > 7) {
        emit_rex(dbuffer, 1, 0, 0, 1);
        regNumber &= 0b111;
    }

    dbuffer_push(dbuffer, 1, 0x50 | regNumber);
}

void emit_popReg(dbuffer_t *dbuffer, reg64 reg) {
    uint8_t regNumber = kReg64Number[reg];
    if (regNumber > 7) {
        emit_rex(dbuffer, 1, 0, 0, 1);
        regNumber &= 0b111;
    }

    dbuffer_push(dbuffer, 1, 0x58 | regNumber);
}

void emit_ret(dbuffer_t *dbuffer) { dbuffer_pushChar(dbuffer, 0xC3); }

void emit_decReg64(dbuffer_t *dbuffer, reg64 reg) {
    uint8_t regNumber = kReg64Number[reg];
    emit_rex(dbuffer, 1, 0, 0, regNumber > 7);
    regNumber &= 0b111;

    dbuffer_pushChar(dbuffer, 0xFF); // dec
    emit_modrm(dbuffer, 3, 1 /* op */, regNumber);
}

void emit_call(dbuffer_t *dbuffer, label_t *label) {
    dbuffer_push(dbuffer, 1, 0xe8);
    relocation_emit(dbuffer, label, RELATIVE, INT32, 4);
}

// -- General code generation system --

// Clean this up.
int _getRealReg(int i) {
    // We don't use RSP, RBP, RBX, R12-15, for register allocation.
    // RAX, RCX, RDX, RSI, RDI, R8, R9, R10, R11
    int realRegs[] = {0, 1, 2, 6, 7, 8, 9, 10, 11, 12};
    return realRegs[i];
}

int arch_getRealReg(struct variable *var) { return _getRealReg(var->reg); }

// RAX, RCX, RDX, RSI, RDI, R8, R9, R10, R11
// int realRegs[] = {0, 1 ,2, 6, 7, 8, 9, 10, 11, 12};

struct variable *arch_initFunctionArg(struct codegen *cg, int i) {
    // %rdi,%rsi,%rdx,%rcx,%r8,%r9
    // reg allocator id's of registers
    int regs[] = {4, 3, 2, 1, 5, 6};
    if (i < 6)
        return codegen_newVarReg(cg, regs[i]);
    return NULL;
}

// load from memory location to the register.
void arch_loadReg(struct codegen *cg, struct variable *var) {
    assert(var->stackPos > 0 && "Can't load a variable with no stack position");

    int rReg = arch_getRealReg(var);
    emit_loadRegRBP64(&cg->buffer, (reg64)rReg, -var->stackPos * 8);
}

void arch_store(struct codegen *cg, struct variable *var) {
    assert(var->stackPos >= 0 && "There must be a stack position.");
    variable_ref(cg, var);
    int rReg = arch_getRealReg(var);
    emit_storeRegRBP64(&cg->buffer, (reg64)rReg, -var->stackPos * 8);
}

void arch_spill(struct codegen *cg, struct variable *var) {
    arch_store(cg, var);
}

void arch_functionCall(struct codegen *cg, label_t *label, size_t count,
                       struct variable **var) {
    assert(count < 7 && "More than 6 args are currently not supported");
    int regs[] = {4, 3, 2, 1, 5, 6};

    for (int i = 0; i < cg->registerCount; i++) {
        struct variable *regVar = cg->registerStatus[i];
        if (regVar != NULL)
            variable_store(cg, regVar);
    }

    for (int i = 0; i < 6; i++) {
        // if the variable is already on the correct register then don't mess
        // with it.

        struct variable *regVar = cg->registerStatus[regs[i]];

        // This register is caller saved, so save it.
        if (i < count) {
            // If there is no register allocated to this variable then load id
            // to the correct one.
            if (var[i]->reg == -1) {
                var[i]->reg = regs[i];
                arch_loadReg(cg, var[i]);

                // no need to update the lru.
                continue;
            }
            // Move the variable to the correct register
            // Wish I was a graph allocator :-(
            emit_storeReg64(&cg->buffer, _getRealReg(var[i]->reg),
                            _getRealReg(regs[i]));
        }
    }

    for (int i = 0; i < count; i++) {
        var[i]->reg = -1;
    }

    emit_call(&cg->buffer, label);
}

// %rdi,%rsi,%rdx,%rcx,%r8, %r9
/*
void emit_testFunction(dbuffer_t *dbuffer, label_t  *putsLabel, label_t
*message) { emit_pushReg(dbuffer, RBP); emit_storeReg64(dbuffer, RSP, RBP);
    emit_subConst64(dbuffer, RSP, 16);


    label_t loopBody = (label_t){};

    emit_storeConst64(dbuffer, RAX, 10);
    emit_storeRegRBP64(dbuffer, RAX, -8);

    label_setOffset(&loopBody, dbuffer->usage);

    emit_storeLabel64(dbuffer, RDI, message);
    emit_call(dbuffer, putsLabel);

    emit_loadRegRBP64(dbuffer, RAX, -8);
    emit_decReg64(dbuffer, RAX);
    emit_storeRegRBP64(dbuffer, RAX, -8);

    emit_checkZero64(dbuffer, RAX);
    emit_jumpZeroRel8(dbuffer, &loopBody);

    emit_storeReg64(dbuffer, RBP, RSP);
    emit_popReg(dbuffer, RBP);
    emit_ret(dbuffer);

    label_apply(&loopBody, dbuffer->buffer);
}
int main(int argc, char *args[]) {
    dbuffer_t dbuffer;
    dbuffer_init(&dbuffer);

    char *msg = "ohalan";

    label_t putsLabel = (label_t){};
    label_t message = (label_t){};
    emit_testFunction(&dbuffer, &putsLabel, &message);

    void *ptr = allocate_executable(dbuffer.usage);
    printf("%p pointer\n", msg);

    label_setOffset(&putsLabel, (unsigned long)puts);
    label_setOffset(&message, msg);
    label_applyMemory(&putsLabel, dbuffer.buffer, (unsigned long)ptr);
    label_applyMemory(&message, dbuffer.buffer, (unsigned long)ptr);

    memcpy(ptr, dbuffer.buffer, dbuffer.usage);
    ((void (*)())ptr)();

    return 0;
}

*/
