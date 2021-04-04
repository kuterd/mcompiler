#include "platform_utils.h"
#include <sys/mman.h>
#include <stddef.h>

void* allocate_executable(size_t size) {
    return mmap(NULL, size,
            PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_ANONYMOUS | MAP_PRIVATE, 0 ,0);   
}
