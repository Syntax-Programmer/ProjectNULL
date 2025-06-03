#pragma once

#include "../common.h"

#define MATCH_TOKEN(tok, value) if (!strcmp(tok, value))
#define STR_TO_BOOL(str)                                                       \
  ((!strcasecmp(str, "true") || !strcmp(str, "1")) ? true : false)

extern bool yaml_ParserParse(const char *yaml_file,
                             bool (*allocator)(void *dest, const char *key,
                                               const char *val, const char *id),
                             void *dest);
