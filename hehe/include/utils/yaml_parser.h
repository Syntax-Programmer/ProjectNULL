#pragma once

#include "../common.h"

extern StatusCode
yaml_ParserParse(const char *yaml_file,
                 StatusCode (*allocator)(void *dest, const CharBuffer key,
                                         const CharBuffer val,
                                         const CharBuffer id, void *extra),
                 void *dest, void *extra);
