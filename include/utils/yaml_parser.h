#pragma once

#include "../common.h"

extern StatusCode yaml_ParserParse(const char *yaml_file,
                                   void (*allocator)(void *dest, String key,
                                                     String val, String id),
                                   void *dest);
