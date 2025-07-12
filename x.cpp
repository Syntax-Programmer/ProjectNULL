#include <iostream>
#include <stdint.h>

int32_t main() {
    int32_t x = 4;

    for (int32_t i = 0; i < x; i++) {
        for (int32_t j = 0; j < i; j++) {
            std::cout << "  ";
        }
        for (int32_t j = 0; j < x - i; j++) {
            std::cout << (i + 1) << " ";
        }
        std::cout << "\n";
    }

    return 0;
}
