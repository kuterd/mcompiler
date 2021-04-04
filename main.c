#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "dbuffer.h"
#include "elf.h"
#include "relocation.h"

void elf64_initIdent(struct elf_ident *ident) {
    ident->signiture[0] = 0x7F;
    ident->signiture[1] = 'E';
    ident->signiture[2] = 'L';
    ident->signiture[3] = 'F';

    ident->eclass = ELF_CLASS_64;    
    ident->endiannes = ELF_LITTLE_ENDIAN;
    ident->version = 1;    
    ident->abi = ELF_LINUX;
    ident->abi_version = 3;
    
    memset(ident->pad, 0, 7); 
}

void emit(dbuffer_t *dbuffer) {
    label_t start;

    struct elf64_header header = (struct elf64_header){};
    elf64_initIdent(&header.ident); 
    
    header.type = ELF_ET_EXEC;    
    header.machine = ELF_AMD64;
    header.e_version = 1;
    header.entry_point = 0;
    header.ph_offset = 0x40; // Right after the header.
    header.header_size = 0x40;
    header.phe_size = sizeof(struct elf64_program);
    header.phe_num = 1;    

    dbuffer_pushData(dbuffer, &header, sizeof(struct elf64_header));
    relocation_set(&start, ABSOLUTE8, 0, offsetof(struct elf64_header, entry_point)); 
    
    struct elf64_program program = (struct elf64_program){};
    
    dbuffer_pushData(dbuffer, &program, sizeof(struct elf64_program));
}    

int main() {
    dbuffer_t dbuffer;
    dbuffer_init(&dbuffer);
//  dbuffer_pushInt(&dbuffer, 0xFFAA, 2);
    emit(&dbuffer);  
    FILE *file = fopen("test.out", "wb");
    if (file == NULL) {
        puts("Failed to open file");
        return 1;
    }
    dbuffer_write(&dbuffer, file);
    
    fclose(file);
    
    return 0;
}
