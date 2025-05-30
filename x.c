#include <math.h>
#include <stdio.h>
#include <stdint.h>

int32_t main() {
    int64_t x = 1;
    for (int32_t i = 1; i < 64; i++) {
        x += (int)ceil(x/2.0);
    }

    printf("%ld\n", x);
}
