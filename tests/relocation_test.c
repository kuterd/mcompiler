#include <stdio.h>

#include "relocation.h"
#include "dbuffer.h"

int main(int argc, char *args[]) {
    dbuffer_t dbuffer;
    dbuffer_init(&dbuffer);
    
    label_t testLabel = (label_t){};
    relocation_set(&testLabel, RELATIVE4, -4, dbuffer.usage); 
    dbuffer_pushLong(&dbuffer, 0, 4);    
    dbuffer_pushChars(&dbuffer, 0, 100);
   
    label_setOffset(&testLabel, dbuffer.usage);        
    label_apply(&testLabel, dbuffer.buffer); 
    

    FILE *file = fopen("test.out", "wb");    

    dbuffer_write(&dbuffer, file);  
    

    return 0; 
}
