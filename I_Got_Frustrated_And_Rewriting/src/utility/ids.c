#include "../../include/utility/ids.h"
#include <stdint.h>

uint64_t id_ASCIISumHash(ID str_id) {
    uint64_t result = 0;

    while (*str_id) {
        result += (uint8_t)(*str_id);
        str_id++;
    }

    return result;
}
