#include "platform_utils.h"
#include "codegen.h"
#include "x86_64.h"

int main() {
    struct codegen cg;
    codegen_init(&cg, 10); 
    label_t putsLabel = (label_t){};
    label_t stackSize = (label_t){};
    label_t startBlock = (label_t){};
    label_t loopBlock = (label_t){};

    struct variable *vars[2];
    codegen_initFunction(&cg, 2, vars);
   
    emit_pushReg(&cg.buffer, RBP);
    emit_storeReg64(&cg.buffer, RSP, RBP);
    emit_subLabel64(&cg.buffer, RSP, &stackSize);  

  
    codegen_pushBlock(&cg, &startBlock);
    struct variable *counter = codegen_newVar(&cg); 
    variable_ref(&cg, counter);
    emit_storeConst64(&cg.buffer, arch_getRealReg(counter), 10);
    codegen_popBlock(&cg);
    codegen_pushBlock(&cg, &loopBlock);

    struct variable *args[] = {vars[0]};
    arch_functionCall(&cg, &putsLabel, 1, args);
   
    variable_ref(&cg, counter);
    
    emit_decReg64(&cg.buffer, arch_getRealReg(counter)); 
    emit_checkZero64(&cg.buffer, arch_getRealReg(counter)); 

    codegen_popBlock(&cg);
    emit_jumpZeroRel8(&cg.buffer, &loopBlock);  

    emit_storeReg64(&cg.buffer, RBP, RSP);
    emit_popReg(&cg.buffer, RBP);
    emit_ret(&cg.buffer);

    void *ptr = allocate_executable(cg.buffer.usage);
    
    label_setOffset(&putsLabel, (unsigned long)puts);
    label_setOffset(&stackSize, cg.frameSize * 8);

    // relative to the memory location. 

    label_applyMemory(&putsLabel, cg.buffer.buffer, (unsigned long)ptr); 
    
    label_apply(&stackSize, cg.buffer.buffer); 
    
    label_apply(&startBlock, cg.buffer.buffer); 
    label_apply(&loopBlock, cg.buffer.buffer); 
    
    FILE *file = fopen("cg_out.out", "wb");
    dbuffer_write(&cg.buffer, file); 
    
    fclose(file);

    memcpy(ptr, cg.buffer.buffer, cg.buffer.usage);
    ((void (*)(char*, int))ptr)("test", 10);

    return 0;
}

/*
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
