#pragma once

#include "../common.h"

extern bool yaml_ParserParse(const char *yaml_file,
                             bool (*allocator)(void *dest, const char *key,
                                               const char *val, const char *id),
                             void *dest);
