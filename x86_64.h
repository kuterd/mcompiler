#include <stdint.h>
#include "utils.h"

// Registers that are bigger than 7 require a REX prefix.

// There is no 1 byte access to non general purpose regiters.
// registers that end with H are not accessible when there is REX. 
#define REGISTER8(o, a) \
    o(a, AL, 0)         \
    o(a, CL, 1)         \
    o(a, DL, 2)         \
    o(a, BL, 3)         \
    o(a, AH, 4)         \
    o(a, CH, 5)         \
    o(a, DH, 6)         \
    o(a, BH, 7)         \
    o(a, SPL, 4)        \
    o(a, BPL, 5)        \
    o(a, SIL, 6)        \
    o(a, DIL, 7)        \
    o(a, R8L, 8)        \
    o(a, R9L, 9)        \
    o(a, R10L, 10)      \
    o(a, R11L, 11)      \
    o(a, R12L, 12)      \
    o(a, R13L, 13)      \
    o(a, R14L, 14)      \
    o(a, R15L, 15)      

#define REGISTER16(o, a)\
    o(a, AX, 0)         \
    o(a, CX, 1)         \
    o(a, DX, 2)         \
    o(a, BX, 3)         \
    o(a, SP, 4)         \
    o(a, BP, 5)         \
    o(a, SI, 6)         \
    o(a, DI, 7)         \
    o(a, R8W, 8)        \
    o(a, R9W, 9)        \
    o(a, R10W, 10)      \
    o(a, R11W, 11)      \
    o(a, R12W, 12)      \
    o(a, R13W, 13)      \
    o(a, R14W, 14)      \
    o(a, R15W, 15)      

#define REGISTER32(o, a) \
    o(a, EAX, 0)         \
    o(a, ECX, 1)         \
    o(a, EDX, 2)         \
    o(a, EBX, 3)         \
    o(a, ESP, 4)         \
    o(a, EBP, 5)         \
    o(a, ESI, 6)         \
    o(a, EDI, 7)         \
    o(a, R8DW, 8)        \
    o(a, R9DW, 9)        \
    o(a, R10DW, 10)      \
    o(a, R11DW, 11)      \
    o(a, R12DW, 12)      \
    o(a, R13DW, 13)      \
    o(a, R14DW, 14)      \
    o(a, R15DW, 15)     

#define REGISTER64(o, a) \
    o(a, RAX, 0)         \
    o(a, RCX, 1)         \
    o(a, RDX, 2)         \
    o(a, RBX, 3)         \
    o(a, RSP, 4)         \
    o(a, RBP, 5)         \
    o(a, RSI, 6)         \
    o(a, RDI, 7)         \
    o(a, R8, 8)          \
    o(a, R9, 9)          \
    o(a, R10, 10)        \
    o(a, R11, 11)        \
    o(a, R12, 12)        \
    o(a, R13, 13)        \
    o(a, R14, 14)        \
    o(a, R15, 15)        


#define FIRST3(a,b,c) a(b)
#define SECOND3(a,b,c) a(c)

typedef enum {
    REGISTER8(FIRST3, COMMA)
} reg8;

typedef enum {
    REGISTER16(FIRST3, COMMA)
} reg16;

typedef enum {
    REGISTER32(FIRST3, COMMA)
} reg32; 

typedef enum {
    REGISTER64(FIRST3, COMMA)
} reg64;

// Register encoding numbers.

uint8_t kReg8Number[] = {
    REGISTER8(SECOND3, COMMA)
};

uint8_t kReg16Number[] = {
    REGISTER16(SECOND3, COMMA)
};

uint8_t kReg32Number[] = {
    REGISTER32(SECOND3, COMMA)
};

uint8_t kReg64Number[] = {
    REGISTER64(SECOND3, COMMA)
};

