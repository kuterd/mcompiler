include_directories(PRIVATE ../)

add_executable(relocation_test relocation_test.c ../utils.c)
add_executable(hashmap_test hashmap_test.c ../list.c ../utils.c)
add_executable(format_test format_test.c ../utils.c)

