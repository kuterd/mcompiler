#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#pragma pack(push, 1)

#define ELF_CLASS_64 2
#define CLASS_32 1

#define ELF_LITTLE_ENDIAN 1
#define ELF_BIG_ENDIAN 2

#define ELF_LINUX 3

#define ELF_ET_EXEC 2

#define ELF_AMD64 0x3E

#define ELF_PT_LOAD 1

struct elf_ident {
    int8_t signiture[4];
    int8_t eclass;
    int8_t endiannes;
    uint8_t version;
    uint8_t abi;
    uint8_t abi_version;
    int8_t pad[7];
};

struct elf64_header {
    struct elf_ident ident;
    uint16_t type;
    uint16_t machine;
    uint32_t e_version;

    uint64_t entry_point;
    uint64_t ph_offset;
    uint64_t sh_offset;

    uint32_t flags;
    uint16_t header_size;
    uint16_t phe_size;
    uint16_t phe_num;
    uint16_t she_size;
    uint16_t she_num;
    uint16_t she_stridx;
};

struct elf64_program {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vaddr;
    uint64_t addr;
    uint64_t fsize;
    uint64_t msize;
    uint64_t allign;
};

struct elf64_section {
    uint32_t name;
    uint32_t type;
    uint64_t flags;
    uint64_t addr;
    uint64_t offset;
    uint64_t size;
    uint32_t link;
    uint32_t info;
    uint64_t addralign;
    uint64_t entsize;
};

#pragma pack(pop)

#endif
