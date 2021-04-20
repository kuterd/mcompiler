include_directories(PRIVATE ../)

add_executable(relocation_test relocation_test.c ../relocation.c ../buffer.c ../utils.c)
add_executable(hashmap_test hashmap_test.c ../buffer.c ../list.c ../utils.c)
add_executable(format_test format_test.c ../buffer.c ../utils.c)
add_executable(tokenizer_test parser_test.c ../buffer.c ../parser.c ../utils.c)
add_executable(ast_dump ast_dump.c ../buffer.c ../relocation.c ../parser.c ../utils.c)
add_executable(codegen_test codegen_test.c ../platform_utils.c ../list.c ../codegen.c ../relocation.c ../x86_64_assembly.c ../buffer.c ../parser.c ../utils.c )


