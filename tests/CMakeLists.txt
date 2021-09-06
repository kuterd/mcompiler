include_directories(PRIVATE ../)

set(general
    ../hashmap.c ../zone_alloc.c ../buffer.c ../list.c
    ../utils.c ../format.c ../parser.c ../relocation.c 
    ../dot_builder.c)

set(ir ${general} ../dominators.c ../ssa_conversion.c ../ir.c ../ir_creation.c)
set(codegen ${ir} ../codegen.c ../x86_64_assembly.c ../platform_utils.c)

add_executable(relocation_test relocation_test.c ${general})
add_executable(hashmap_test hashmap_test.c ${general})
add_executable(format_test format_test.c ${general})
add_executable(tokenizer_test parser_test.c ${general})
add_executable(ast_dump ast_dump.c ${general})
add_executable(ir_test ir_test.c ${ir})
add_executable(codegen_test codegen_test.c ${codegen})
add_executable(zone_test zone_alloc_test.c ${general})
add_executable(cfg_test cfg_test.c ${ir})
add_executable(ir_conversion ir_conversion.c ${ir})
add_executable(ssa_test ssa_test.c ${ir})
