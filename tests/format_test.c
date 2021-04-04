#include <stdio.h>
#include "format.h"

int main() {
    format("Kuter {range}, {range}\n", RANGE_STRING("Deneme"), RANGE_STRING("1 2 3")); 
}
