#include "format.h"
#include <stdio.h>

int main() {
    format_print("Kuter {int},{str},{range}\n", 10, "test",
                 RANGE_STRING("1 2 3"));
}
